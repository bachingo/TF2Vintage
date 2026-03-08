//go:build windows

package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"syscall"
)

// newCmd returns an exec.Cmd with HideWindow set so spawned processes don't
// flash a console window on Windows.
func newCmd(name string, args ...string) *exec.Cmd {
	cmd := exec.Command(name, args...)
	cmd.SysProcAttr = &syscall.SysProcAttr{HideWindow: true}
	return cmd
}

// swapBinDir replaces liveBinDir with stagingBinDir on Windows.
//
// On Windows, atomicSwapDir would fail with "file in use" because the running
// tf2vintage-updater.exe lives inside liveBinDir and Windows locks open
// executables against directory renames. The workaround:
//
//  1. Copy the running exe to a temp file outside both directories.
//  2. Perform the normal atomic rename swap (liveBinDir → .old, staging → live).
//     The old directory containing the locked exe is now .old — Windows permits
//     renaming a directory that contains a locked file as long as the file itself
//     is not being renamed, so step 2 succeeds.
//  3. Copy the saved exe back into the new liveBinDir, overwriting the copy that
//     arrived from the zip (same binary — checksums match — but we need an
//     unlocked file handle to write it).
//  4. Schedule deletion of the temp file via a detached cmd.exe ping loop so it
//     is cleaned up after this process exits.
//
// The .old directory is removed in step 2 by atomicSwapDir. The locked exe
// inside it is released when this process exits, at which point Windows allows
// the directory to be deleted by the next updater run's RemoveAll call.
func swapBinDir(liveBinDir, stagingBinDir string) error {
	exe, err := os.Executable()
	if err != nil {
		return fmt.Errorf("could not locate own executable: %v", err)
	}

	// Step 1 — save a copy of the running exe to a temp file beside the mod dir
	// (same drive as staging, so the copy is fast and on the same filesystem).
	tmpExe := filepath.Join(filepath.Dir(filepath.Dir(liveBinDir)), "tf2vintage-updater.tmp.exe")
	if err := copyFile(exe, tmpExe); err != nil {
		return fmt.Errorf("could not save updater copy before swap: %v", err)
	}

	// Step 2 — atomic rename swap; the running exe stays locked inside .old but
	// Windows allows renaming the parent directory, so this succeeds.
	if err := atomicSwapDir(liveBinDir, stagingBinDir); err != nil {
		os.Remove(tmpExe)
		return err
	}

	// Step 3 — put the updater exe back in the new live bin dir.
	// The zip already placed a copy there; we overwrite it with our saved version
	// to ensure the on-disk file is not locked (the zip's copy was written during
	// extraction, not from a running process, so it's already unlocked — but
	// writing our own copy keeps the two bytes identical and avoids any edge case
	// where the zip carried a different build of the updater).
	updaterDest := filepath.Join(liveBinDir, updaterName())
	if err := copyFile(tmpExe, updaterDest); err != nil {
		// Non-fatal: the zip's copy is already in place and valid.
		termWarn("Could not refresh updater exe after swap: %v — existing copy kept", err)
	}

	// Step 4 — schedule cleanup of the temp file after this process exits.
	scheduleDelete(tmpExe)
	return nil
}

