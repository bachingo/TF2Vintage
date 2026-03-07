//go:build linux

package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"time"
)

func isWindows() bool { return false }

// ── Steam path detection ──────────────────────────────────────────────────────

func findSteamPath() (string, error) {
	candidates := []string{
		filepath.Join(os.Getenv("HOME"), ".steam", "steam"),
		filepath.Join(os.Getenv("HOME"), ".local", "share", "Steam"),
	}
	for _, p := range candidates {
		if _, err := os.Stat(p); err == nil {
			return p, nil
		}
	}
	return "", fmt.Errorf("Steam installation not found")
}

func findSourcemodsPath() (string, error) {
	steam, err := findSteamPath()
	if err != nil {
		return "", err
	}
	return filepath.Join(steam, "steamapps", "sourcemods"), nil
}

// ── Steam process management ──────────────────────────────────────────────────

func closeSteam() error {
	cmd := exec.Command("pkill", "-x", "steam")
	cmd.Run() // ignore error — steam may not be running
	time.Sleep(2 * time.Second)
	return nil
}

func relaunchSteam(steamPath string) error {
	steamBin := filepath.Join(steamPath, "steam.sh")
	cmd := exec.Command(steamBin)
	cmd.Start()
	return nil
}

func isSteamRunning() bool {
	out, err := exec.Command("pgrep", "-x", "steam").Output()
	return err == nil && len(out) > 0
}

// createDesktopShortcutPlatform writes a .desktop file to ~/Desktop (or
// $XDG_DESKTOP_DIR if set) that runs the updater then launches TF2 Vintage.
func createDesktopShortcutPlatform(updaterPath string) error {
	desktopDir := os.Getenv("XDG_DESKTOP_DIR")
	if desktopDir == "" {
		desktopDir = filepath.Join(os.Getenv("HOME"), "Desktop")
	}
	if err := os.MkdirAll(desktopDir, 0755); err != nil {
		return fmt.Errorf("could not access desktop directory: %v", err)
	}

	shortcutPath := filepath.Join(desktopDir, "tf2vintage.desktop")
	content := "[Desktop Entry]\n" +
		"Version=1.0\n" +
		"Type=Application\n" +
		"Name=TF2 Vintage\n" +
		"Comment=Launch TF2 Vintage (checks for updates first)\n" +
		"Exec=" + updaterPath + " %U\n" +
		"Terminal=false\n" +
		"Categories=Game;\n"

	if err := os.WriteFile(shortcutPath, []byte(content), 0755); err != nil {
		return fmt.Errorf("could not write desktop shortcut: %v", err)
	}
	return nil
}
