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
		runUpdateMode(exe)
	} else {
		runInstallMode()
	}
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
