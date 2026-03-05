package main

import (
	"bufio"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"time"
)

// ── Steam path detection ──────────────────────────────────────────────────────

func findSteamPath() (string, error) {
	if runtime.GOOS == "windows" {
		return findSteamPathWindows()
	}
	return findSteamPathLinux()
}

func findSteamPathLinux() (string, error) {
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

// findSourcemodsPath returns the sourcemods directory for the current platform.
func findSourcemodsPath() (string, error) {
	if runtime.GOOS == "windows" {
		return findSourcemodsPathWindows()
	}
	steam, err := findSteamPathLinux()
	if err != nil {
		return "", err
	}
	return filepath.Join(steam, "steamapps", "sourcemods"), nil
}

// ── SDK Base 2013 MP detection ────────────────────────────────────────────────

const sdkAppID = "243750"

func isSDKInstalled(steamPath string) bool {
	manifestPath := filepath.Join(steamPath, "steamapps", fmt.Sprintf("appmanifest_%s.acf", sdkAppID))
	_, err := os.Stat(manifestPath)
	return err == nil
}

func promptInstallSDK() {
	openURL(fmt.Sprintf("steam://install/%s", sdkAppID))
}

func openURL(url string) {
	var cmd *exec.Cmd
	if runtime.GOOS == "windows" {
		cmd = exec.Command("rundll32", "url.dll,FileProtocolHandler", url)
	} else {
		cmd = exec.Command("xdg-open", url)
	}
	cmd.Start()
}

// ── Steam process management ──────────────────────────────────────────────────

func closeSteam() error {
	if runtime.GOOS == "windows" {
		return closeSteamWindows()
	}
	return closeSteamLinux()
}

func closeSteamLinux() error {
	cmd := exec.Command("pkill", "-x", "steam")
	cmd.Run() // ignore error — steam may not be running
	time.Sleep(2 * time.Second)
	return nil
}

func relaunchSteam(steamPath string) error {
	if runtime.GOOS == "windows" {
		return relaunchSteamWindows(steamPath)
	}
	return relaunchSteamLinux(steamPath)
}

func relaunchSteamLinux(steamPath string) error {
	steamBin := filepath.Join(steamPath, "steam.sh")
	cmd := exec.Command(steamBin)
	cmd.Start()
	return nil
}

// ── VDF read/write for launch options ────────────────────────────────────────

// setLaunchOption writes the updater launch option for SDK Base 2013 MP
// into Steam's localconfig.vdf. Steam must be closed before calling this.
func setLaunchOption(steamPath, updaterPath string) error {
	vdfPath, err := findLocalConfig(steamPath)
	if err != nil {
		return err
	}

	data, err := os.ReadFile(vdfPath)
	if err != nil {
		return fmt.Errorf("could not read localconfig.vdf: %v", err)
	}

	// Parse the full VDF tree
	nodes, err := vdfParse(string(data))
	if err != nil {
		return fmt.Errorf("could not parse localconfig.vdf: %v", err)
	}

	launchOption := fmt.Sprintf(`"%s" %%command%%`, updaterPath)

	// Path: UserLocalConfigStore → Software → Valve → Steam → Apps → 243750 → LaunchOptions
	vdfSet(&nodes, launchOption,
		"UserLocalConfigStore", "Software", "Valve", "Steam", "Apps", sdkAppID, "LaunchOptions",
	)

	// Write back
	output := vdfSerialize(nodes, 0)
	return os.WriteFile(vdfPath, []byte(output), 0644)
}

func findLocalConfig(steamPath string) (string, error) {
	usersPath := filepath.Join(steamPath, "userdata")
	entries, err := os.ReadDir(usersPath)
	if err != nil {
		return "", fmt.Errorf("could not read userdata directory: %v", err)
	}

	// Use the most recently modified user directory
	var newest string
	var newestTime int64
	for _, e := range entries {
		if !e.IsDir() {
			continue
		}
		candidate := filepath.Join(usersPath, e.Name(), "config", "localconfig.vdf")
		info, err := os.Stat(candidate)
		if err != nil {
			continue
		}
		if info.ModTime().Unix() > newestTime {
			newestTime = info.ModTime().Unix()
			newest = candidate
		}
	}

	if newest == "" {
		return "", fmt.Errorf("no localconfig.vdf found — is Steam signed in?")
	}
	return newest, nil
}



// ── Helpers ───────────────────────────────────────────────────────────────────

func waitForSDKInstall(steamPath string) bool {
	// If Steam isn't running, the steam:// URI fires into the void.
	// Launch Steam first so it's ready to handle the install request.
	if !isSteamRunning() {
		fmt.Println("Steam is not running — launching Steam first...")
		relaunchSteam(steamPath)
		time.Sleep(5 * time.Second) // give Steam time to start
		// Re-fire the install URI now that Steam is up
		promptInstallSDK()
	}

	// SDK is 3.29 GB — allow up to 45 minutes on slow connections
	fmt.Println("Waiting for Source SDK Base 2013 Multiplayer to finish installing (3.29 GB)...")
	fmt.Println("This may take up to 45 minutes depending on your connection speed.")
	for i := 0; i < 900; i++ { // 900 × 3s = 45 minutes
		time.Sleep(3 * time.Second)
		if isSDKInstalled(steamPath) {
			fmt.Println("Source SDK Base 2013 Multiplayer installed.")
			return true
		}
		// Print a dot every 30 seconds so the user knows it's still working
		if i%10 == 9 {
			elapsed := (i + 1) * 3
			fmt.Printf("  Still waiting... %dm%ds elapsed\n", elapsed/60, elapsed%60)
		}
	}
	return false
}

func isSteamRunning() bool {
	if runtime.GOOS == "windows" {
		return isSteamRunningWindows()
	}
	out, err := exec.Command("pgrep", "-x", "steam").Output()
	return err == nil && len(out) > 0
}

func readLine() string {
	scanner := bufio.NewScanner(os.Stdin)
	scanner.Scan()
	return strings.TrimSpace(scanner.Text())
}
