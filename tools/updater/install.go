package main

import (
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"runtime"
)

// InstallState tracks progress for the GUI to display.
type InstallState struct {
	Status   string
	Progress float64 // 0.0–1.0
	Done     bool
	Err      error
}

type installNeeds struct {
	freshInstall bool
	repairBase   bool
	repairBin    bool
	sourcemods   string
	installDir   string
	reason       string
}

func runInstallMode() {
	startInstallUI(doInstall)
}

// doInstall is called by the GUI on a background goroutine.
// askAltPath blocks until the GUI responds with a path (empty = use default).
func doInstall(report func(InstallState), askAltPath func() string) {
	report(InstallState{Status: "Locating Steam..."})

	steamPath, err := findSteamPath()
	if err != nil {
		openURL("https://store.steampowered.com/about/")
		report(InstallState{Err: fmt.Errorf(
			"Steam could not be found on this computer.\n\n" +
				"If Steam is installed in a non-standard location, place the tf2vintage-updater " +
				"into your existing tf2vintage/bin/" + binDirName() + "/ folder and run it from there instead.\n\n" +
				"Otherwise, install Steam from:\nhttps://store.steampowered.com/about/")})
		return
	}

	// ── Check / install Source SDK Base 2013 MP ───────────────────────────────
	report(InstallState{Status: "Checking Source SDK Base 2013 Multiplayer...", Progress: 0.05})
	if !isSDKInstalled(steamPath) {
		report(InstallState{
			Status: "Source SDK Base 2013 Multiplayer not found.\n" +
				"Opening Steam to install it — please wait for it to finish (3.29 GB)..."})
		promptInstallSDK()
		if !waitForSDKInstall(steamPath) {
			openURL("https://store.steampowered.com/app/243750/Source_SDK_Base_2013_Multiplayer/")
			report(InstallState{Err: fmt.Errorf(
				"Source SDK Base 2013 Multiplayer did not finish installing.\n\n" +
					"The Steam store page has been opened in your browser.\n" +
					"Once it is installed (3.29 GB), run this installer again.")})
			return
		}
	}

	// ── Check / install Team Fortress 2 (app 440) ─────────────────────────────
	// TF2V mounts TF2's content directory at runtime and overlays its own assets
	// on top. TF2 must be installed for the game to have its full content.
	report(InstallState{Status: "Checking Team Fortress 2...", Progress: 0.08})
	if !isTF2Installed(steamPath) {
		report(InstallState{
			Status: "Team Fortress 2 not found.\n" +
				"Opening Steam to install it — please wait for it to finish (~25 GB)..."})
		promptInstallTF2()
		if !waitForTF2Install(steamPath) {
			openURL("https://store.steampowered.com/app/440/Team_Fortress_2/")
			report(InstallState{Err: fmt.Errorf(
				"Team Fortress 2 did not finish installing.\n\n" +
					"The Steam store page has been opened in your browser.\n" +
					"Once it is installed (~25 GB), run this installer again.")})
			return
		}
	}

	// ── Find Sourcemods path ──────────────────────────────────────────────────
	report(InstallState{Status: "Checking existing installation...", Progress: 0.10})
	sourcemods, err := findSourcemodsPath()
	if err != nil {
		report(InstallState{Err: fmt.Errorf("Could not locate Sourcemods folder: %v", err)})
		return
	}

	// ── Ask user about alternate install location ─────────────────────────────
	// This blocks until the GUI's folder picker is resolved (or dismissed).
	altPath := askAltPath()

	installDir, err := resolveInstallDir(sourcemods, altPath)
	if err != nil {
		report(InstallState{Err: err})
		return
	}

	// ── Diagnose existing state ───────────────────────────────────────────────
	needs := diagnose(sourcemods, installDir)
	report(InstallState{Status: needs.reason, Progress: 0.12})

	// Healthy install — just run the updater logic
	if !needs.freshInstall && !needs.repairBase && !needs.repairBin {
		report(InstallState{Status: "Existing installation looks healthy — checking for updates...", Progress: 0.15})
		latest, err := fetchLatestRelease()
		if err != nil {
			report(InstallState{Err: fmt.Errorf("Could not reach GitHub: %v", err)})
			return
		}
		binDir := platformBinDir(installDir)
		existingCfg := loadConfig(binDir)
		// For an already-installed copy, use a staging dir beside the install
		// so updates can be swapped in atomically.
		stagingRoot := installDir + ".staging"
		stagingBinDir := filepath.Join(stagingRoot, "bin", binDirName())
		stagingModDir := filepath.Join(stagingRoot, "mod")
		os.RemoveAll(stagingRoot)
		os.MkdirAll(stagingBinDir, 0755)
		os.MkdirAll(stagingModDir, 0755)

		binUpdated := false
		baseUpdated := false

		switch err := updateInstall(installDir, stagingRoot, stagingBinDir, stagingModDir, latest); {
		case err == nil:
			binUpdated = true
			baseUpdated = true
		case errors.Is(err, errUpToDate):
			// nothing staged — do not swap
		default:
			report(InstallState{Err: fmt.Errorf("Update failed: %v", err)})
			os.RemoveAll(stagingRoot)
			return
		}
		if existingCfg.DownloadSymbols {
			switch err := updateSymbols(binDir, stagingBinDir, latest); {
			case err == nil:
				binUpdated = true // symbols staged alongside bin — swap together
			case errors.Is(err, errUpToDate):
				// nothing staged — do not swap
			default:
				termWarn("Symbol update failed: %v", err)
			}
		}
		if binUpdated {
			if err := atomicSwapDir(binDir, stagingBinDir); err != nil {
				report(InstallState{Err: fmt.Errorf("Bin swap failed: %v", err)})
				os.RemoveAll(stagingRoot)
				return
			}
		}
		if baseUpdated {
			if err := atomicSwapDir(installDir, stagingModDir); err != nil {
				report(InstallState{Err: fmt.Errorf("Base swap failed: %v", err)})
				os.RemoveAll(stagingRoot)
				return
			}
		}
		os.RemoveAll(stagingRoot)
		finalize(report, steamPath, filepath.Join(binDir, updaterName()), false)
		return
	}

	if err := os.MkdirAll(installDir, 0755); err != nil {
		report(InstallState{Err: fmt.Errorf("Could not create install directory: %v", err)})
		return
	}

	// ── Fetch release info ────────────────────────────────────────────────────
	report(InstallState{Status: "Fetching latest release information...", Progress: 0.15})
	latest, err := fetchLatestRelease()
	if err != nil {
		report(InstallState{Err: fmt.Errorf(
			"Could not reach GitHub: %v\n\nCheck your internet connection and try again.", err)})
		return
	}

	// ── Download + extract full package (base + bin) ─────────────────────────
	// Both repairBase and repairBin draw from the combined tf2vintage-full.zip,
	// which packages the complete game-asset tree and both platform bin trees
	// together. Using the unified updateInstall path keeps repair consistent with
	// the normal update path and avoids the stale single-platform bin asset.
	report(InstallState{Status: "Downloading full package — this may take a while...", Progress: 0.20})
	repairStagingRoot := installDir + ".staging"
	repairStagingBinDir := filepath.Join(repairStagingRoot, "bin", binDirName())
	repairStagingModDir := filepath.Join(repairStagingRoot, "mod")
	os.RemoveAll(repairStagingRoot)
	os.MkdirAll(repairStagingBinDir, 0755)
	os.MkdirAll(repairStagingModDir, 0755)

	// Force a full download by temporarily clearing the local manifest so
	// updateInstall treats this as a fresh install (case 1).
	localManifestPath := filepath.Join(installDir, "base-manifest.json")
	savedManifest, _ := os.ReadFile(localManifestPath)
	os.Remove(localManifestPath)

	switch err := updateInstall(installDir, repairStagingRoot, repairStagingBinDir, repairStagingModDir, latest); {
	case err == nil:
		// staged — swap into place below
	case errors.Is(err, errUpToDate):
		// nothing to do (shouldn't happen after removing the manifest, but be safe)
	default:
		if len(savedManifest) > 0 {
			os.WriteFile(localManifestPath, savedManifest, 0644)
		}
		os.RemoveAll(repairStagingRoot)
		report(InstallState{Err: fmt.Errorf("Download failed: %v\n\nRun the installer again to retry.", err)})
		return
	}

	report(InstallState{Status: "Applying repair...", Progress: 0.85})
	repairBinDir := platformBinDir(installDir)
	if err := atomicSwapDir(repairBinDir, repairStagingBinDir); err != nil {
		os.RemoveAll(repairStagingRoot)
		report(InstallState{Err: fmt.Errorf("Bin repair swap failed: %v", err)})
		return
	}
	if err := atomicSwapDir(installDir, repairStagingModDir); err != nil {
		os.RemoveAll(repairStagingRoot)
		report(InstallState{Err: fmt.Errorf("Base repair swap failed: %v", err)})
		return
	}
	os.RemoveAll(repairStagingRoot)

	cfg := UpdaterConfig{DownloadSymbols: false}

	// ── Copy updater into install location ────────────────────────────────────
	report(InstallState{Status: "Installing updater...", Progress: 0.88})
	exe, _ := os.Executable()
	binDir := platformBinDir(installDir)
	updaterDest := filepath.Join(binDir, updaterName())

	if err := copyFile(exe, updaterDest); err != nil {
		report(InstallState{Err: fmt.Errorf(
			"Failed to install updater: %v\n\n"+
				"If TF2 Vintage is already running, close it and try again.", err)})
		return
	}
	if runtime.GOOS != "windows" {
		os.Chmod(updaterDest, 0755)
	}

	// Save config so the updater remembers symbol preference on future runs
	if err := saveConfig(binDir, cfg); err != nil {
		termWarn("Could not save updater config: %v", err)
	}

	// Download symbols immediately if opted in
	if cfg.DownloadSymbols {
		report(InstallState{Status: "Downloading debug symbols...", Progress: 0.89})
		if latest != nil {
			// Fresh install: binDir is live and staging simultaneously — no distinction needed.
			switch err := updateSymbols(binDir, binDir, latest); {
			case err == nil, errors.Is(err, errUpToDate):
				// ok
			default:
				termWarn("Symbol download failed: %v — you can retry with --enable-symbols", err)
			}
		}
	}

	finalize(report, steamPath, updaterDest, true)

	// Remove the downloaded installer from wherever the user ran it from
	if exe != updaterDest {
		scheduleDelete(exe)
	}
}

// ── Diagnosis ─────────────────────────────────────────────────────────────────

func diagnose(sourcemods, installDir string) installNeeds {
	needs := installNeeds{sourcemods: sourcemods, installDir: installDir}

	if _, err := os.Stat(installDir); os.IsNotExist(err) {
		needs.freshInstall = true
		needs.repairBase = true
		needs.repairBin = true
		needs.reason = "No existing installation found — performing fresh install."
		return needs
	}

	manifest := loadLocalManifest(filepath.Join(installDir, "base-manifest.json"))
	if manifest == nil || manifest.Tag == "" {
		needs.repairBase = true
	}
	for _, dir := range []string{"maps", "materials", "models", "sound"} {
		if _, err := os.Stat(filepath.Join(installDir, dir)); os.IsNotExist(err) {
			needs.repairBase = true
			break
		}
	}

	if err := validateBinDir(platformBinDir(installDir)); err != nil {
		needs.repairBin = true
	}

	switch {
	case needs.repairBase && needs.repairBin:
		needs.reason = "Installation appears incomplete or corrupted — repairing base and binaries."
	case needs.repairBase:
		needs.reason = "Base assets are missing or corrupted — repairing base."
	case needs.repairBin:
		needs.reason = "Game binaries are missing or corrupted — repairing binaries."
	default:
		needs.reason = "Existing installation looks healthy."
	}
	return needs
}

// ── Post-install ──────────────────────────────────────────────────────────────

// finalize creates the desktop shortcut and, on a fresh first install only,
// restarts Steam so it registers the new sourcemod and generates its AppID.
// Repairs and updates pass restartSteam=false — Steam is already aware of the
// installation and does not need to rescan.
func finalize(report func(InstallState), steamPath, updaterPath string, restartSteam bool) {
	report(InstallState{Status: "Creating desktop shortcut...", Progress: 0.94})
	if err := createDesktopShortcut(updaterPath); err != nil {
		termWarn("Could not create desktop shortcut: %v", err)
		report(InstallState{
			Status:   "Desktop shortcut could not be created — launch TF2 Vintage from Steam instead.",
			Progress: 0.95,
		})
	}

	if restartSteam {
		report(InstallState{Status: "Restarting Steam to register TF2 Vintage...", Progress: 0.97})
		if err := closeSteam(); err != nil {
			termWarn("Could not close Steam: %v", err)
		}
		relaunchSteam(steamPath)
		report(InstallState{
			Status:   "Installation complete!\nSteam is restarting — TF2 Vintage will appear in your library shortly.",
			Progress: 1.0,
			Done:     true,
		})
		return
	}

	report(InstallState{
		Status:   "Done!\nA desktop shortcut has been created for TF2 Vintage.",
		Progress: 1.0,
		Done:     true,
	})
}

// ── Helpers ───────────────────────────────────────────────────────────────────

func updaterName() string {
	if runtime.GOOS == "windows" {
		return "tf2vintage-updater.exe"
	}
	return "tf2vintage-updater"
}

func copyFile(src, dst string) error {
	in, err := os.Open(src)
	if err != nil {
		return err
	}
	defer in.Close()
	if err := os.MkdirAll(filepath.Dir(dst), 0755); err != nil {
		return err
	}
	out, err := os.Create(dst)
	if err != nil {
		return err
	}
	defer out.Close()
	_, err = copyIO(in, out)
	return err
}
