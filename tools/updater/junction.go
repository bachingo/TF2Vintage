package main

import (
	"fmt"
	"os"
	"path/filepath"
	"runtime"
)

// resolveInstallDir determines where tf2vintage files actually get written.
// If the user chose an alternate path, it:
//   1. Creates the target directory at altPath/tf2vintage
//   2. Creates a junction (Windows) or symlink (Linux) from
//      sourcemods/tf2vintage → altPath/tf2vintage
//
// Returns the real install directory and an error if setup failed.
func resolveInstallDir(sourcemods, altPath string) (installDir string, err error) {
	junctionPath := filepath.Join(sourcemods, "tf2vintage")

	if altPath == "" {
		// No alternate path — install directly into Sourcemods as normal
		return junctionPath, nil
	}

	installDir = filepath.Join(altPath, "tf2vintage")

	// Create the real install directory on the target drive
	if err := os.MkdirAll(installDir, 0755); err != nil {
		return "", fmt.Errorf("could not create install directory on %s: %v", altPath, err)
	}

	// Remove any existing junction/folder at the Sourcemods path
	if info, err := os.Lstat(junctionPath); err == nil {
		if isJunctionOrSymlink(info) {
			os.Remove(junctionPath)
		} else if info.IsDir() {
			// Real directory exists — don't silently clobber it
			return "", fmt.Errorf(
				"%s already exists as a real directory.\n"+
					"Move or delete it first if you want to install to %s instead.",
				junctionPath, altPath)
		}
	}

	// Create junction (Windows) or symlink (Linux)
	if err := createLink(junctionPath, installDir); err != nil {
		return "", fmt.Errorf("could not create junction from %s → %s: %v",
			junctionPath, installDir, err)
	}

	fmt.Printf("Junction created: %s → %s\n", junctionPath, installDir)
	return installDir, nil
}

// isJunctionOrSymlink returns true if the file info indicates a symlink or
// Windows directory junction (both appear as ModeSymlink on Go's os package).
func isJunctionOrSymlink(info os.FileInfo) bool {
	return info.Mode()&os.ModeSymlink != 0
}

// createLink creates a directory junction on Windows or a symlink on Linux.
func createLink(linkPath, targetPath string) error {
	if runtime.GOOS == "windows" {
		return createJunctionWindows(linkPath, targetPath)
	}
	return os.Symlink(targetPath, linkPath)
}
