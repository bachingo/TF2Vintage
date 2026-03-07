//go:build windows

package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
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
	return len(out) > 0 && !strings.Contains(string(out), "No tasks")
}

// createDesktopShortcutPlatform writes a Windows internet shortcut (.url) to
// the user's Desktop. Double-clicking it runs the updater, which checks for
// updates and then launches TF2 Vintage via the Steam rungameid URI.
//
// .url format is a standard INI file recognised by Windows Explorer.
// The IconFile line points the shortcut at the updater's own icon.
func createDesktopShortcutPlatform(updaterPath string) error {
	// SHGetFolderPath would be ideal but requires cgo; USERPROFILE + Desktop
	// works on every Windows version since XP and covers non-English installs.
	desktop := filepath.Join(os.Getenv("USERPROFILE"), "Desktop")
	if _, err := os.Stat(desktop); os.IsNotExist(err) {
		// Fallback: OneDrive-backed Desktop on some Windows 11 setups
		desktop = filepath.Join(os.Getenv("USERPROFILE"), "OneDrive", "Desktop")
	}
	if err := os.MkdirAll(desktop, 0755); err != nil {
		return fmt.Errorf("could not access desktop directory: %v", err)
	}

	shortcutPath := filepath.Join(desktop, "TF2 Vintage.url")
	// The URL field runs the updater directly. Windows will execute it as a
	// program because the scheme is not http/https.
	content := "[InternetShortcut]\r\n" +
		"URL=file:///" + filepath.ToSlash(updaterPath) + "\r\n" +
		"IconFile=" + updaterPath + "\r\n" +
		"IconIndex=0\r\n"

	if err := os.WriteFile(shortcutPath, []byte(content), 0644); err != nil {
		return fmt.Errorf("could not write desktop shortcut: %v", err)
	}
	return nil
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
