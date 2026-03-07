package main

import (
	"bufio"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"
)

// ── App ID constants ──────────────────────────────────────────────────────────

const (
	sdkAppID = "243750"
	tf2AppID = "440"

	// tf2vintageAppIDUnsigned is the Steam app ID for the tf2vintage sourcemod,
	// computed as CRC32("tf2vintage") | 0x80000000 = 3369364862.
	// Steam derives sourcemod app IDs deterministically from the folder name.
	//
	// The 64-bit ID shown in the Steam UI (e.g. 14471311890598574118) includes the
	// user's SteamID3 in the upper 32 bits — that part varies per account.
	// The localconfig.vdf key is only the lower 32 bits, but Steam versions differ
	// on whether they store it as the unsigned decimal (3369364862) or the signed
	// decimal (-925602434). We write both to cover all cases.
	tf2vintageAppIDUnsigned = "3369364862"
	tf2vintageAppIDSigned   = "-925602434"
)

// ── Steam library folder discovery ───────────────────────────────────────────

// findSteamLibraries returns the steamapps directory for every configured
// Steam library, including the one inside the main Steam install directory.
//
// Steam records all library paths in <steamPath>/steamapps/libraryfolders.vdf.
// Two VDF formats exist:
//
//	Modern (post-2021): numbered sections, each with a "path" leaf
//	  "0" { "path"  "C:\\Program Files (x86)\\Steam" … }
//	  "1" { "path"  "F:\\SteamLibrary" … }
//
//	Legacy (pre-2021): numbered keys mapping directly to the path string
//	  "1"  "F:\\SteamLibrary"
//
// On any read or parse error the main library is still returned, so callers
// always get at least one candidate.
func findSteamLibraries(steamPath string) []string {
	main := filepath.Join(steamPath, "steamapps")
	libs := []string{main}

	data, err := os.ReadFile(filepath.Join(main, "libraryfolders.vdf"))
	if err != nil {
		return libs
	}
	nodes, err := vdfParse(string(data))
	if err != nil {
		return libs
	}

	// Root is a single top-level section ("libraryfolders" / "LibraryFolders").
	var entries []*vdfNode
	if len(nodes) == 1 && nodes[0].Children != nil {
		entries = nodes[0].Children
	} else {
		entries = nodes
	}

	seen := map[string]bool{filepath.ToSlash(strings.ToLower(main)): true}

	for _, entry := range entries {
		// Only process numeric keys (library index entries).
		if len(entry.Key) == 0 || entry.Key[0] < '0' || entry.Key[0] > '9' {
			continue
		}
		var libPath string
		if entry.Children != nil {
			// Modern format: section containing a "path" leaf.
			for _, child := range entry.Children {
				if strings.EqualFold(child.Key, "path") && child.Children == nil {
					libPath = filepath.FromSlash(child.Value)
					break
				}
			}
		} else if entry.Value != "" {
			// Legacy format: numbered key → path string.
			libPath = filepath.FromSlash(entry.Value)
		}
		if libPath == "" {
			continue
		}
		steamapps := filepath.Join(libPath, "steamapps")
		key := filepath.ToSlash(strings.ToLower(steamapps))
		if !seen[key] {
			seen[key] = true
			libs = append(libs, steamapps)
		}
	}
	return libs
}

// isAppInstalled returns true if a Steam app's appmanifest file is found in
// any configured Steam library folder, including libraries on other drives.
func isAppInstalled(steamPath, appID string) bool {
	filename := fmt.Sprintf("appmanifest_%s.acf", appID)
	for _, lib := range findSteamLibraries(steamPath) {
		if _, err := os.Stat(filepath.Join(lib, filename)); err == nil {
			return true
		}
	}
	return false
}

// ── SDK Base 2013 MP detection ────────────────────────────────────────────────

func isSDKInstalled(steamPath string) bool {
	return isAppInstalled(steamPath, sdkAppID)
}

func promptInstallSDK() {
	openURL(fmt.Sprintf("steam://install/%s", sdkAppID))
}

// ── Team Fortress 2 (app 440) detection ──────────────────────────────────────

// isTF2Installed checks whether Team Fortress 2 is installed in any Steam
// library. TF2V mounts TF2's content directory at runtime and overlays its own
// assets on top — without TF2 present the game will be missing most content.
func isTF2Installed(steamPath string) bool {
	return isAppInstalled(steamPath, tf2AppID)
}

func promptInstallTF2() {
	openURL(fmt.Sprintf("steam://install/%s", tf2AppID))
}

func waitForTF2Install(steamPath string) bool {
	if !isSteamRunning() {
		fmt.Println("Steam is not running — launching Steam first...")
		relaunchSteam(steamPath)
		time.Sleep(5 * time.Second)
		promptInstallTF2()
	}

	// TF2 is ~25 GB — allow up to 3 hours on slow connections
	fmt.Println("Waiting for Team Fortress 2 to finish installing (~25 GB)...")
	fmt.Println("This may take up to 3 hours depending on your connection speed.")
	for i := 0; i < 3600; i++ { // 3600 × 3s = 3 hours
		time.Sleep(3 * time.Second)
		if isTF2Installed(steamPath) {
			fmt.Println("Team Fortress 2 installed.")
			return true
		}
		if i%10 == 9 {
			elapsed := (i + 1) * 3
			fmt.Printf("  Still waiting... %dm%ds elapsed\n", elapsed/60, elapsed%60)
		}
	}
	return false
}

func openURL(url string) {
	var cmd *exec.Cmd
	if isWindows() {
		cmd = exec.Command("rundll32", "url.dll,FileProtocolHandler", url)
	} else {
		cmd = exec.Command("xdg-open", url)
	}
	cmd.Start()
}

// ── VDF read/write for launch options ────────────────────────────────────────

// setLaunchOption writes the updater launch option for TF2 Vintage into Steam's
// localconfig.vdf. Steam must be closed before calling this.
//
// The launch option must target TF2 Vintage's own sourcemod app ID, not
// sdkAppID (243750). Steam assigns sourcemods their own deterministic app IDs
// (see tf2vintageAppIDUnsigned above), and that is the entry the user sees in
// their library and launches from.
//
// Steam versions differ on whether localconfig.vdf stores the 32-bit app ID as
// unsigned or signed decimal, so we write both representations.
func setLaunchOption(steamPath, updaterPath string) error {
	vdfPath, err := findLocalConfig(steamPath)
	if err != nil {
		return err
	}

	data, err := os.ReadFile(vdfPath)
	if err != nil {
		return fmt.Errorf("could not read localconfig.vdf: %v", err)
	}

	nodes, err := vdfParse(string(data))
	if err != nil {
		return fmt.Errorf("could not parse localconfig.vdf: %v", err)
	}

	launchOption := fmt.Sprintf(`"%s" %%command%%`, updaterPath)

	// Write under both the unsigned and signed decimal representations of the
	// sourcemod app ID. vdfSet is idempotent — whichever key already exists in
	// the file will be updated; the other will be created if absent.
	vdfSet(&nodes, launchOption,
		"UserLocalConfigStore", "Software", "Valve", "Steam", "Apps", tf2vintageAppIDUnsigned, "LaunchOptions",
	)
	vdfSet(&nodes, launchOption,
		"UserLocalConfigStore", "Software", "Valve", "Steam", "Apps", tf2vintageAppIDSigned, "LaunchOptions",
	)

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

func readLine() string {
	scanner := bufio.NewScanner(os.Stdin)
	scanner.Scan()
	return strings.TrimSpace(scanner.Text())
}
