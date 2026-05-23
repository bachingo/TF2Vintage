package main

import (
	"encoding/hex"
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"strings"
)

// validateCommitSHA returns true if s looks like a git commit SHA
// (40 hex chars for full SHA, or 7–9 for short SHA).
func validateCommitSHA(s string) bool {
	s = strings.TrimSpace(s)
	if len(s) < 7 || len(s) > 40 {
		return false
	}
	_, err := hex.DecodeString(s)
	return err == nil
}

// validateBinDir checks that binDir contains at least one .dll or .so file
// with a non-zero size, and that version-bin.txt has a valid commit SHA.
func validateBinDir(binDir string) error {
	// Check version-bin.txt
	commit := readField(filepath.Join(binDir, "version-bin.txt"), "commit")
	if !validateCommitSHA(commit) {
		return fmt.Errorf("version-bin.txt missing or has invalid commit SHA")
	}

	// Check at least one binary exists with non-zero size
	entries, err := os.ReadDir(binDir)
	if err != nil {
		return fmt.Errorf("bin directory unreadable: %v", err)
	}
	for _, e := range entries {
		ext := strings.ToLower(filepath.Ext(e.Name()))
		if ext == ".dll" || ext == ".so" {
			info, err := e.Info()
			if err == nil && info.Size() > 0 {
				return nil // found at least one valid binary
			}
		}
	}
	return fmt.Errorf("no valid game binaries found in bin/%s", binDirName())
}

// backupBinDir copies binDir to binDir+".bak", overwriting any existing backup.
func backupBinDir(binDir string) error {
	backupDir := binDir + ".bak"
	os.RemoveAll(backupDir)
	return copyDir(binDir, backupDir)
}

// restoreBinDir replaces binDir with the backup created by backupBinDir.
func restoreBinDir(binDir string) error {
	backupDir := binDir + ".bak"
	if _, err := os.Stat(backupDir); os.IsNotExist(err) {
		return fmt.Errorf("no backup found at %s", backupDir)
	}
	os.RemoveAll(binDir)
	return copyDir(backupDir, binDir)
}

func copyDir(src, dst string) error {
	return filepath.WalkDir(src, func(path string, d os.DirEntry, err error) error {
		if err != nil {
			return err
		}
		rel, err := filepath.Rel(src, path)
		if err != nil {
			return err
		}
		target := filepath.Join(dst, rel)
		if d.IsDir() {
			return os.MkdirAll(target, 0755)
		}
		return copyFile(path, target)
	})
}

// validateModRoot checks that critical files exist in the mod root after extraction
func validateModRoot(modDir string) error {
    var criticalFiles []string
    
    // Files that must exist on all platforms
    criticalFiles = append(criticalFiles,
        filepath.Join(modDir, "gameinfo.txt"),
        filepath.Join(modDir, "base-manifest.json"),
    )
    
    // Platform-specific executables
    if runtime.GOOS == "windows" {
        criticalFiles = append(criticalFiles,
            filepath.Join(modDir, "tf2vintage_win64.exe"),
            filepath.Join(modDir, "tf2vintage-updater.exe"),
        )
    } else {
        criticalFiles = append(criticalFiles,
            filepath.Join(modDir, "launcher_tf2vintage"),
            filepath.Join(modDir, "tf2vintage-updater"),
        )
    }
    
    var missing []string
    for _, path := range criticalFiles {
        if _, err := os.Stat(path); os.IsNotExist(err) {
            missing = append(missing, filepath.Base(path))
        }
    }
    
    if len(missing) > 0 {
        return fmt.Errorf("critical files missing after extraction: %s", strings.Join(missing, ", "))
    }
    
    return nil
}