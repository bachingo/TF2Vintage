package main

import (
	"fmt"
	"os"
	"path/filepath"
)

// updateSymbols downloads and extracts tf2vintage-symbols.zip if the symbols
// are not already current. liveBinDir is the live install location — used for
// the version check and disk space check, both of which need to reflect the
// actual installed state. stagingBinDir is where the symbols are extracted to;
// atomicSwapDir will move them into place alongside the rest of the bin dir.
// Pass the same path for both during a fresh install where there is no
// live/staging distinction yet.
//
// tf2vintage-symbols.zip contains a flat symbols/ directory with .pdb (Windows)
// and .debug (Linux) files together in the same folder, plus a symbols-build.txt
// version stamp. The updater extracts the whole tree regardless of platform —
// debuggers on either OS simply point at bin/<platform>/symbols/ and find what
// they need.
func updateSymbols(liveBinDir, stagingBinDir string, latest *ghRelease) error {
	// Version check reads from the live install — staging is always empty
	liveSymbolsDir := filepath.Join(liveBinDir, "symbols")
	localTag := readField(filepath.Join(liveSymbolsDir, "symbols-version.txt"), "tag")
	if localTag == latest.TagName {
		fmt.Println("Symbols are up to date.")
		return errUpToDate
	}

	fmt.Printf("Symbol update: %s → %s\n", shortOrNone(localTag), latest.TagName)

	url := assetURL(latest, "tf2vintage-symbols.zip")
	if url == "" {
		termWarn("Symbol package not found in release — skipping symbol update.")
		return errUpToDate
	}

	// Disk space check against live dir (same filesystem as staging)
	if err := checkDiskSpace(liveBinDir, minFreeBytesForSymbols); err != nil {
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

	// Extract the entire symbols/ subtree into staging — both platforms' files
	// (.pdb and .debug) live flat in one folder so any debugger can point at it.
	fmt.Println("Extracting symbols...")
	stagingSymbolsDir := filepath.Join(stagingBinDir, "symbols")
	if err := os.MkdirAll(stagingSymbolsDir, 0755); err != nil {
		os.Remove(tmp)
		return fmt.Errorf("could not create symbols directory: %v", err)
	}

	if err := extractZipSubdir(tmp, stagingSymbolsDir, "symbols"); err != nil {
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

