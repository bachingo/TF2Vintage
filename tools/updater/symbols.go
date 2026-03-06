package main

import (
	"fmt"
	"os"
	"path/filepath"
	"runtime"
)

// updateSymbols downloads and extracts tf2vintage-symbols.zip if the symbols
// are not already current. liveBinDir is the live install location — used for
// the version check and disk space check, both of which need to reflect the
// actual installed state. stagingBinDir is where the symbols are extracted to;
// atomicSwapDir will move them into place alongside the rest of the bin dir.
// Pass the same path for both during a fresh install where there is no
// live/staging distinction yet.
func updateSymbols(liveBinDir, stagingBinDir string, latest *ghRelease) error {
	// Version check reads from the live install — staging is always empty
	liveSymbolsDir := filepath.Join(liveBinDir, "symbols")
	localTag := readField(filepath.Join(liveSymbolsDir, "symbols-version.txt"), "tag")
	if localTag == latest.TagName {
		fmt.Println("Symbols are up to date.")
		return nil
	}

	fmt.Printf("Symbol update: %s → %s\n", shortOrNone(localTag), latest.TagName)

	url := assetURL(latest, "tf2vintage-symbols.zip")
	if url == "" {
		termWarn("Symbol package not found in release — skipping symbol update.")
		return nil
	}

	// Disk space check against live dir (same filesystem as staging)
	if err := checkDiskSpace(liveBinDir, 256*1024*1024); err != nil {
		return fmt.Errorf("not enough disk space for symbols: %v", err)
	}

	fmt.Println("Downloading debug symbols...")
	tmp, err := downloadWithProgress(url)
	if err != nil {
		return fmt.Errorf("symbol download failed: %v", err)
	}

	// Verify checksum before extracting
	if expectedHash := fetchAssetChecksum(latest, "tf2vintage-symbols.zip"); expectedHash != "" {
		if err := verifyDownloadChecksum(tmp, expectedHash); err != nil {
			os.Remove(tmp)
			return fmt.Errorf("symbol download integrity check failed: %v", err)
		}
	}

	// Extract platform-specific subtree into staging — no need for both platforms
	fmt.Println("Extracting symbols...")
	stagingSymbolsDir := filepath.Join(stagingBinDir, "symbols")
	if err := os.MkdirAll(stagingSymbolsDir, 0755); err != nil {
		os.Remove(tmp)
		return fmt.Errorf("could not create symbols directory: %v", err)
	}

	platformSubdir := symbolsPlatformSubdir()
	if err := extractZipSubdir(tmp, stagingSymbolsDir, platformSubdir); err != nil {
		os.Remove(tmp)
		return fmt.Errorf("symbol extraction failed: %v", err)
	}
	os.Remove(tmp)

	// Write version marker into staging — moves into the live location when
	// atomicSwapDir commits the bin dir, so the next run skips re-downloading.
	versionFile := filepath.Join(stagingSymbolsDir, "symbols-version.txt")
	if err := os.WriteFile(versionFile, []byte(fmt.Sprintf("tag=%s\n", latest.TagName)), 0644); err != nil {
		return fmt.Errorf("could not write symbols version file: %v", err)
	}

	fmt.Printf("Symbols staged to %s\n", stagingSymbolsDir)
	return nil
}

func symbolsPlatformSubdir() string {
	if runtime.GOOS == "windows" {
		return "symbols/windows"
	}
	return "symbols/linux"
}

// extractZipSubdir extracts only entries under subdir prefix into destDir,
// stripping the subdir prefix from extracted paths.
func extractZipSubdir(zipPath, destDir, subdir string) error {
	prefix := filepath.ToSlash(subdir)
	if prefix != "" && prefix[len(prefix)-1] != '/' {
		prefix += "/"
	}
	return extractZipFiltered(zipPath, destDir, func(name string) (string, bool) {
		name = filepath.ToSlash(name)
		if prefix == "" {
			return name, true
		}
		if name == prefix[:len(prefix)-1] {
			return "", false // skip the directory entry itself
		}
		if len(name) > len(prefix) && name[:len(prefix)] == prefix {
			return name[len(prefix):], true // strip prefix
		}
		return "", false
	})
}
