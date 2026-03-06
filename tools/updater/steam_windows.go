//go:build windows

package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"syscall"
	"time"

	"golang.org/x/sys/windows/registry"
)

func isWindows() bool { return true }

// ── Steam path detection ──────────────────────────────────────────────────────

func findSteamPath() (string, error) {
	return findSteamPathWindows()
}

func findSteamPathWindows() (string, error) {
	k, err := registry.OpenKey(registry.CURRENT_USER, `Software\Valve\Steam`, registry.QUERY_VALUE)
	if err != nil {
		k, err = registry.OpenKey(registry.LOCAL_MACHINE, `Software\Valve\Steam`, registry.QUERY_VALUE)
		if err != nil {
			return "", fmt.Errorf("Steam not found in registry")
		}
	}
	defer k.Close()
	// Valve has used both "SteamPath" and "InstallPath" across Steam versions.
	path, _, err := k.GetStringValue("SteamPath")
	if err != nil {
		// Fallback for older or alternate Steam installs
		path, _, err = k.GetStringValue("InstallPath")
		if err != nil {
			return "", fmt.Errorf("Steam path not found in registry (tried SteamPath and InstallPath): %v", err)
		}
	}
	return filepath.FromSlash(path), nil
}

func findSourcemodsPath() (string, error) {
	return findSourcemodsPathWindows()
}

func findSourcemodsPathWindows() (string, error) {
	steam, err := findSteamPathWindows()
	if err != nil {
		return "", err
	}
	return filepath.Join(steam, "steamapps", "sourcemods"), nil
}

// ── Steam process management ──────────────────────────────────────────────────

func closeSteam() error {
	return closeSteamWindows()
}

func closeSteamWindows() error {
	cmd := exec.Command("taskkill", "/IM", "steam.exe", "/F")
	cmd.SysProcAttr = &syscall.SysProcAttr{HideWindow: true}
	cmd.Run()
	time.Sleep(2 * time.Second)
	return nil
}

func relaunchSteam(steamPath string) error {
	return relaunchSteamWindows(steamPath)
}

func relaunchSteamWindows(steamPath string) error {
	steamExe := filepath.Join(steamPath, "steam.exe")
	cmd := exec.Command(steamExe)
	cmd.SysProcAttr = &syscall.SysProcAttr{HideWindow: false}
	return cmd.Start()
}

func isSteamRunning() bool {
	return isSteamRunningWindows()
}

func isSteamRunningWindows() bool {
	out, err := exec.Command("tasklist", "/FI", "IMAGENAME eq steam.exe", "/NH").Output()
	if err != nil {
		return false
	}
	return len(out) > 0 && !contains(string(out), "No tasks")
}

func contains(s, sub string) bool {
	return len(s) >= len(sub) && (s == sub || len(s) > 0 && indexStr(s, sub) >= 0)
}

func indexStr(s, sub string) int {
	for i := 0; i <= len(s)-len(sub); i++ {
		if s[i:i+len(sub)] == sub {
			return i
		}
	}
	return -1
}

// ── VDF launch option: Windows needs Steam path from registry ─────────────────

func findSteamUserDataPath() (string, error) {
	steam, err := findSteamPathWindows()
	if err != nil {
		return "", err
	}
	return filepath.Join(steam, "userdata"), nil
}

// writeLaunchOption writes %command% launch options into localconfig.vdf.
// Kept here so it can use Windows-specific registry path if needed.
func writeLaunchOptionPlatform(updaterPath string) error {
	steam, err := findSteamPathWindows()
	if err != nil {
		return err
	}
	return setLaunchOption(steam, updaterPath)
}

// ── Self-delete helpers ───────────────────────────────────────────────────────

// selfDeleteWindows schedules the installer exe to delete itself via a detached cmd.
func selfDeleteWindows(exePath string) {
	script := fmt.Sprintf(`ping 127.0.0.1 -n 3 > nul & del /F /Q "%s"`, exePath)
	cmd := exec.Command("cmd", "/C", script)
	cmd.SysProcAttr = &syscall.SysProcAttr{
		HideWindow:    true,
		CreationFlags: syscall.CREATE_NEW_PROCESS_GROUP,
	}
	cmd.Start()
	os.Exit(0)
}

// ── GUI helpers ───────────────────────────────────────────────────────────────

// openDirectoryWindows opens a folder in Windows Explorer.
func openDirectoryWindows(path string) {
	exec.Command("explorer", path).Start()
}
