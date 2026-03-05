package main

import (
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"io"
	"net/http"
	"os"
	"strings"
)

const checksumAsset = "checksums.sha256"

// verifyUpdaterChecksum fetches checksums.sha256 from the latest release and
// verifies that the running executable matches the published hash.
// Returns nil if the hash matches or if no checksum file is available.
func verifyUpdaterChecksum(latest *ghRelease) error {
	url := assetURL(latest, checksumAsset)
	if url == "" {
		// No checksum file published — skip verification (older releases)
		return nil
	}

	resp, err := http.Get(url)
	if err != nil {
		termWarn("Could not fetch checksum file: %v — skipping verification", err)
		return nil
	}
	defer resp.Body.Close()

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return fmt.Errorf("could not read checksum file: %v", err)
	}

	exe, err := os.Executable()
	if err != nil {
		return fmt.Errorf("could not locate own executable: %v", err)
	}

	exeName := strings.ToLower(strings.ReplaceAll(exe, "\\", "/"))
	exeBase := exeName[strings.LastIndex(exeName, "/")+1:]

	// Parse "sha256hash  filename" lines
	expected := ""
	for _, line := range strings.Split(string(body), "\n") {
		line = strings.TrimSpace(line)
		if line == "" || strings.HasPrefix(line, "#") {
			continue
		}
		fields := strings.Fields(line)
		if len(fields) < 2 {
			continue
		}
		// Match on filename only, case-insensitive
		if strings.ToLower(fields[1]) == exeBase ||
			strings.ToLower(strings.TrimPrefix(fields[1], "*")) == exeBase {
			expected = strings.ToLower(fields[0])
			break
		}
	}

	if expected == "" {
		// Our binary name not listed — nothing to verify against
		return nil
	}

	actual, err := sha256File(exe)
	if err != nil {
		return fmt.Errorf("could not hash executable: %v", err)
	}

	if actual != expected {
		return fmt.Errorf(
			"checksum mismatch — this copy of tf2vintage-updater may be corrupted or tampered with.\n\n"+
				"Expected: %s\n"+
				"Got:      %s\n\n"+
				"Please re-download from: https://github.com/%s/%s/releases/latest",
			expected, actual, repoOwner, repoName)
	}

	return nil
}

// verifyDownloadChecksum checks a downloaded file against a known SHA-256 hash.
func verifyDownloadChecksum(path, expectedHex string) error {
	actual, err := sha256File(path)
	if err != nil {
		return fmt.Errorf("could not hash file: %v", err)
	}
	if actual != strings.ToLower(expectedHex) {
		return fmt.Errorf("download checksum mismatch\nExpected: %s\nGot:      %s", expectedHex, actual)
	}
	return nil
}

// fetchAssetChecksum looks up the SHA-256 for a specific asset name from the
// release's checksums.sha256 file. Returns empty string if not found.
func fetchAssetChecksum(latest *ghRelease, assetName string) string {
	url := assetURL(latest, checksumAsset)
	if url == "" {
		return ""
	}
	resp, err := http.Get(url)
	if err != nil {
		return ""
	}
	defer resp.Body.Close()
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return ""
	}
	for _, line := range strings.Split(string(body), "\n") {
		fields := strings.Fields(strings.TrimSpace(line))
		if len(fields) < 2 {
			continue
		}
		if strings.ToLower(strings.TrimPrefix(fields[1], "*")) == strings.ToLower(assetName) {
			return strings.ToLower(fields[0])
		}
	}
	return ""
}

func sha256File(path string) (string, error) {
	f, err := os.Open(path)
	if err != nil {
		return "", err
	}
	defer f.Close()
	h := sha256.New()
	if _, err := io.Copy(h, f); err != nil {
		return "", err
	}
	return hex.EncodeToString(h.Sum(nil)), nil
}
