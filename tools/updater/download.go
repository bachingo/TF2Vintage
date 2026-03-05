package main

import (
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"time"
)

// Minimum free disk space required before downloading.
// Base: ~961MB unpacked + ~350MB compressed + 512MB buffer
// Bins: ~60MB unpacked + 128MB buffer
const (
	minFreeBytesForBase = (961 + 350 + 512) * 1024 * 1024
	minFreeBytesForBins = (60 + 128) * 1024 * 1024
)

// downloadResumable downloads url to destPath, resuming an interrupted
// download if destPath already exists and the server supports Range requests.
func downloadResumable(url, destPath string, cb func(downloaded, total int64)) error {
	var existingSize int64
	if info, err := os.Stat(destPath); err == nil {
		existingSize = info.Size()
	}

	// HEAD to get total size and check Range support
	head, err := http.Head(url)
	if err != nil {
		return fmt.Errorf("could not reach download server: %v", err)
	}
	head.Body.Close()

	totalSize := head.ContentLength
	acceptsRanges := head.Header.Get("Accept-Ranges") == "bytes"

	// Already complete
	if existingSize > 0 && existingSize == totalSize {
		if cb != nil {
			cb(totalSize, totalSize)
		}
		return nil
	}

	// Start fresh if server doesn't support ranges or file is oversized
	if !acceptsRanges || existingSize > totalSize {
		os.Remove(destPath)
		existingSize = 0
	}

	flags := os.O_CREATE | os.O_WRONLY
	if existingSize > 0 {
		flags |= os.O_APPEND
		fmt.Printf("Resuming from %.1f MB...\n", float64(existingSize)/1024/1024)
	}

	f, err := os.OpenFile(destPath, flags, 0644)
	if err != nil {
		return fmt.Errorf("could not open download file: %v", err)
	}
	defer f.Close()

	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return err
	}
	if existingSize > 0 {
		req.Header.Set("Range", fmt.Sprintf("bytes=%d-", existingSize))
	}

	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return fmt.Errorf("download failed: %v", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK && resp.StatusCode != http.StatusPartialContent {
		return fmt.Errorf("unexpected HTTP status: %s", resp.Status)
	}

	downloaded := existingSize
	buf := make([]byte, 256*1024) // 256KB chunks
	lastCall := time.Now()

	for {
		n, err := resp.Body.Read(buf)
		if n > 0 {
			if _, werr := f.Write(buf[:n]); werr != nil {
				return fmt.Errorf("write error — disk full? %v", werr)
			}
			downloaded += int64(n)
			if cb != nil && time.Since(lastCall) > 250*time.Millisecond {
				cb(downloaded, totalSize)
				lastCall = time.Now()
			}
		}
		if err == io.EOF {
			break
		}
		if err != nil {
			// Partial file kept on disk — next run will resume
			return fmt.Errorf("connection interrupted at %.1f MB — will resume next run", float64(downloaded)/1024/1024)
		}
	}

	if cb != nil {
		cb(downloaded, totalSize)
	}
	return nil
}

// downloadWithProgressCallback downloads url to a stable temp path (keyed by
// URL hash) so interrupted downloads can be resumed on the next run.
func downloadWithProgressCallback(url string, cb func(downloaded, total int64)) (string, error) {
	tmpPath := filepath.Join(os.TempDir(), "tf2v-"+urlHash(url)+".tmp")
	if err := downloadResumable(url, tmpPath, cb); err != nil {
		return "", err
	}
	return tmpPath, nil
}

func downloadWithProgress(url string) (string, error) {
	return downloadWithProgressCallback(url, func(downloaded, total int64) {
		printProgress(downloaded, total)
	})
}

func printProgress(downloaded, total int64) {
	if total > 0 {
		fmt.Printf("\r  %.1f MB / %.1f MB (%.0f%%)   ",
			float64(downloaded)/1024/1024,
			float64(total)/1024/1024,
			float64(downloaded)/float64(total)*100,
		)
	} else {
		fmt.Printf("\r  %.1f MB downloaded   ", float64(downloaded)/1024/1024)
	}
}

func checkDiskSpace(path string, minBytes int64) error {
	available, err := availableDiskSpace(path)
	if err != nil {
		termWarn("Could not check disk space: %v", err)
		return nil
	}
	if available < minBytes {
		return fmt.Errorf(
			"not enough disk space: %.1f GB available, %.1f GB needed",
			float64(available)/1024/1024/1024,
			float64(minBytes)/1024/1024/1024,
		)
	}
	return nil
}

func urlHash(url string) string {
	h := uint32(2166136261)
	for i := 0; i < len(url); i++ {
		h ^= uint32(url[i])
		h *= 16777619
	}
	return strconv.FormatUint(uint64(h), 16)
}
