package main

import (
	"fmt"
	"os"
	"path/filepath"
	"runtime"
)

// updateSymbols downloads and extracts tf2vintage-symbols.zip if the user
// has opted in via updater.cfg. Symbols land in bin/x64/symbols/ and are
// keyed to the current release so crash reports always match.
func updateSymbols(binDir string, latest *ghRelease) error {
	symbolsDir := filepath.Join(binDir, "symbols")

	// Check if already up to date
	localTag := readField(filepath.Join(symbolsDir, "symbols-version.txt"), "tag")
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

	// Check disk space — symbols can be 50–200MB extracted
	if err := checkDiskSpace(binDir, 256*1024*1024); err != nil {
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

	// Extract platform-specific subtree only — no need for both platforms
	fmt.Println("Extracting symbols...")
	if err := os.MkdirAll(symbolsDir, 0755); err != nil {
		return fmt.Errorf("could not create symbols directory: %v", err)
	}

	platformSubdir := symbolsPlatformSubdir()
	if err := extractZipSubdir(tmp, symbolsDir, platformSubdir); err != nil {
		os.Remove(tmp)
		return fmt.Errorf("symbol extraction failed: %v", err)
	}
	os.Remove(tmp)

	// Write version marker so we don't re-download on next run
	versionFile := filepath.Join(symbolsDir, "symbols-version.txt")
	os.WriteFile(versionFile, []byte(fmt.Sprintf("tag=%s\n", latest.TagName)), 0644)

	fmt.Printf("Symbols installed to %s\n", symbolsDir)
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
	// Normalise subdir to use forward slashes with trailing slash
	prefix := filepath.ToSlash(subdir)
	if prefix != "" && prefix[len(prefix)-1] != '/' {
		prefix += "/"
	}
	return extractZipFiltered(zipPath, destDir, func(name string) (string, bool) {
		name = filepath.ToSlash(name)
		if prefix == "" {
			return name, true
		}
		// Accept both "symbols/windows/foo.pdb" and just the prefix itself
		if name == prefix[:len(prefix)-1] {
			return "", false // skip the directory entry itself
		}
		if len(name) > len(prefix) && name[:len(prefix)] == prefix {
			return name[len(prefix):], true // strip prefix
		}
		return "", false
	})
}
