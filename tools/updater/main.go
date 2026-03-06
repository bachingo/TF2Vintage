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

// isInstalledPath returns true if the executable is running from inside a
// tf2vintage/bin/x64 (Windows) or tf2vintage/bin/linux64 (Linux) directory,
// which indicates update mode rather than fresh-install mode.
func isInstalledPath(dir string) bool {
	lower := strings.ToLower(filepath.ToSlash(dir))
	suffix := "tf2vintage/bin/" + strings.ToLower(binDirName())
	return strings.HasSuffix(lower, suffix) ||
		strings.Contains(lower, suffix+"/")
}

// platformBinDir returns the path to the platform-specific bin subdirectory
// relative to the mod root. Used by update and install logic.
func platformBinDir(modDir string) string {
	return filepath.Join(modDir, "bin", binDirName())
}

// modDirFromExe walks up from the exe path to the mod root.
// exe lives at:  modDir/bin/<binDirName>/tf2vintage-updater[.exe]
// so modDir is three levels up.
func modDirFromExe(exe string) string {
	return filepath.Dir(filepath.Dir(filepath.Dir(exe)))
}

// currentPlatform returns a short string for display ("windows" / "linux").
func currentPlatform() string {
	if runtime.GOOS == "windows" {
		return "windows"
	}
	return "linux"
}
