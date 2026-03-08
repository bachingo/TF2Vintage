package main

import (
	"os"
	"path/filepath"
	"runtime"
	"strings"
)

func main() {
	exe, err := os.Executable()
	if err != nil {
		termFatal("Could not locate executable: %v", err)
	}
	exeDir := filepath.ToSlash(filepath.Dir(exe))

	if isInstalledPath(exeDir) {
		// Running from inside an existing tf2vintage/bin/<platform>/ — update mode.
		runUpdateMode(exe)
		return
	}

	// Not running from the installed location. Before showing the install UI,
	// check whether TF2 Vintage is already installed somewhere on this system.
	// This covers users who downloaded the updater to their desktop and want
	// it to behave as an updater/repairer rather than triggering a fresh install.
	if installedExe := findExistingInstall(); installedExe != "" {
		runUpdateMode(installedExe)
		return
	}

	runInstallMode()
}

// findExistingInstall searches common locations for an existing tf2vintage
// installation and returns the path to the installed updater exe if found,
// or an empty string if no installation can be located.
//
// Search order:
//  1. sourcemods/tf2vintage  (standard install, possibly a junction/symlink)
//  2. Junction/symlink target  (user installed to an alternate drive)
//
// The returned path is the *installed* updater executable so that
// runUpdateMode can derive modDir correctly via modDirFromExe.
func findExistingInstall() string {
	sourcemods, err := findSourcemodsPath()
	if err != nil {
		// Steam not found — can't search sourcemods, skip straight to install.
		return ""
	}

	// Candidate: sourcemods/tf2vintage (direct install or junction root)
	candidates := []string{filepath.Join(sourcemods, "tf2vintage")}

	// If the sourcemods entry is a junction or symlink, also check the real target.
	junctionPath := filepath.Join(sourcemods, "tf2vintage")
	if info, err := os.Lstat(junctionPath); err == nil {
		if info.Mode()&os.ModeSymlink != 0 {
			if target, err := os.Readlink(junctionPath); err == nil {
				candidates = append(candidates, target)
			}
		}
	}

	updaterExe := "tf2vintage-updater"
	if runtime.GOOS == "windows" {
		updaterExe = "tf2vintage-updater.exe"
	}

	for _, modDir := range candidates {
		candidate := filepath.Join(modDir, "bin", binDirName(), updaterExe)
		if _, err := os.Stat(candidate); err == nil {
			return candidate
		}
	}

	return ""
}

// binDirName returns the platform bin subdirectory name under bin/.
//
//	Windows → "x64"
//	Linux   → "linux64"
func binDirName() string {
	if runtime.GOOS == "windows" {
		return "x64"
	}
	return "linux64"
}

// isInstalledPath returns true when the exe is running from inside a
// tf2vintage/bin/x64 (Windows) or tf2vintage/bin/linux64 (Linux) directory,
// which means we are in update mode rather than fresh-install mode.
func isInstalledPath(dir string) bool {
	lower := strings.ToLower(filepath.ToSlash(dir))
	suffix := "tf2vintage/bin/" + strings.ToLower(binDirName())
	return strings.HasSuffix(lower, suffix) ||
		strings.Contains(lower, suffix+"/")
}

// platformBinDir returns the full path to the platform-specific bin subdir.
func platformBinDir(modDir string) string {
	return filepath.Join(modDir, "bin", binDirName())
}

// modDirFromExe walks three levels up from the exe to the mod root:
//
//	modDir/bin/<binDirName>/tf2vintage-updater[.exe]
func modDirFromExe(exe string) string {
	return filepath.Dir(filepath.Dir(filepath.Dir(exe)))
}
