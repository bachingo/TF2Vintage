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

// swapBinDir atomically replaces liveDir with stagingDir on Windows.
//
// The parameters are now the full mod directory and the full staging root
// (not just the bin subdirectory). On Windows, atomicSwapDir would fail with
// "access denied" when the running tf2vintage-updater.exe is open inside
// liveDir, because Windows locks open executables. However, Windows *does*
// allow renaming a directory that merely *contains* a locked file, as long as
// the file itself is not being renamed directly. The workaround:
//
//  1. Copy the running exe to a temp file outside both directories (but on
//     the same drive so the copy is fast).
//  2. Perform the normal atomic rename swap (liveDir → .old, stagingDir → live).
//     The old directory (containing the locked exe) is now .old; renaming its
//     parent is permitted, so this succeeds.
//  3. Copy the saved exe back into the new liveDir (mod root), overwriting the
//     copy that arrived from the zip. This ensures the on-disk exe is not in a
//     locked state for any subsequent write.
//  4. Schedule deletion of the temp file after this process exits.
//
// The .old directory is removed by atomicSwapDir. The locked exe inside it is
// released when this process exits, after which Windows lets the next updater
// run delete it via RemoveAll.
func swapBinDir(liveDir, stagingDir string) error {
	exe, err := os.Executable()
	if err != nil {
		return fmt.Errorf("could not locate own executable: %v", err)
	}

	// Step 1 — save a copy of the running exe to a temp file beside liveDir
	// (same filesystem parent, so it is guaranteed to be on the same drive).
	tmpExe := filepath.Join(filepath.Dir(liveDir), "tf2vintage-updater.tmp.exe")
	if err := copyFile(exe, tmpExe); err != nil {
		return fmt.Errorf("could not save updater copy before swap: %v", err)
	}

	// Step 2 — atomic rename swap; the locked exe stays inside .old.
	if err := atomicSwapDir(liveDir, stagingDir); err != nil {
		os.Remove(tmpExe)
		return err
	}

	// Step 3 — restore the updater exe into the new mod root.
	// The zip already placed a fresh copy there; we overwrite it with our own
	// saved copy to guarantee the file handle is not locked.
	updaterDest := filepath.Join(liveDir, updaterName())
	if err := copyFile(tmpExe, updaterDest); err != nil {
		// Non-fatal: the zip's copy is already in place and valid.
		termWarn("Could not refresh updater exe after swap: %v — existing copy kept", err)
	}

	// Step 4 — schedule cleanup of the temp file after this process exits.
	scheduleDelete(tmpExe)
	return nil
}

