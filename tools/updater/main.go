package main

import (
	"os"
	"path/filepath"
	"strings"
)

func main() {
	exe, err := os.Executable()
	if err != nil {
		termFatal("Could not locate executable: %v", err)
	}
	exeDir := filepath.ToSlash(filepath.Dir(exe))

	// Determine mode by checking if we're already inside tf2vintage/bin/x64
	if isInstalledPath(exeDir) {
		runUpdateMode(exe)
	} else {
		runInstallMode()
	}
}

// isInstalledPath returns true if the executable is running from
// inside a tf2vintage/bin/x64 directory — indicating update mode.
func isInstalledPath(dir string) bool {
	lower := strings.ToLower(filepath.ToSlash(dir))
	return strings.HasSuffix(lower, "tf2vintage/bin/x64") ||
		strings.Contains(lower, "tf2vintage/bin/x64/")
}
