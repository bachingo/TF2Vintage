package main

import (
	"fmt"
	"os"
	"path/filepath"
	"runtime"
)

// InstallState tracks progress for the GUI to display.
type InstallState struct {
	Status       string
	Progress     float64 // 0.0–1.0
	Done         bool
	Err          error
	ManualLaunch string // non-empty: VDF failed, show this string for manual setup
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
				"If Steam is installed in a non-standard location, place tf2vintage-updater.exe " +
				"into your existing tf2vintage\\bin\\x64\\ folder and run it from there instead.\n\n" +
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
		binDir := filepath.Join(installDir, "bin", "x64")
		if err := updateBin(installDir, binDir, latest); err != nil {
			report(InstallState{Err: fmt.Errorf("Update failed: %v", err)})
			return
		}
		if err := updateBase(installDir, latest); err != nil {
			report(InstallState{Err: fmt.Errorf("Update failed: %v", err)})
			return
		}
		finalize(report, steamPath, filepath.Join(binDir, updaterName()))
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

	// ── Download + extract base ───────────────────────────────────────────────
	if needs.repairBase {
		if err := checkDiskSpace(installDir, minFreeBytesForBase); err != nil {
			report(InstallState{Err: err})
			return
		}

		baseURL := assetURL(latest, "tf2vintage-base.zip")
		if baseURL == "" {
			report(InstallState{Err: fmt.Errorf("Base package not found in latest release — please try again later.")})
			return
		}

		report(InstallState{Status: "Downloading base assets — 350 MB compressed, may take a while...", Progress: 0.20})
		baseTmp, err := downloadWithProgressCallback(baseURL, func(downloaded, total int64) {
			if total > 0 {
				pct := float64(downloaded) / float64(total)
				report(InstallState{
					Status:   fmt.Sprintf("Downloading base assets... %.0f%%", pct*100),
					Progress: 0.20 + pct*0.45,
				})
			}
		})
		if err != nil {
			report(InstallState{Err: fmt.Errorf(
				"Download interrupted: %v\n\nRun the installer again to resume.", err)})
			return
		}
		defer os.Remove(baseTmp)

		// Verify base integrity before extracting
		if expectedHash := fetchAssetChecksum(latest, "tf2vintage-base.zip"); expectedHash != "" {
			if err := verifyDownloadChecksum(baseTmp, expectedHash); err != nil {
				os.Remove(baseTmp)
				report(InstallState{Err: fmt.Errorf("Base download integrity check failed: %v", err)})
				return
			}
		}

		report(InstallState{Status: "Extracting base assets...", Progress: 0.65})
		if err := extractZip(baseTmp, installDir); err != nil {
			report(InstallState{Err: fmt.Errorf("Failed to extract base assets: %v", err)})
			return
		}
		if manifest, err := fetchReleaseManifest(latest); err == nil {
			saveManifest(filepath.Join(installDir, "base-manifest.json"), manifest)
		}
	}

	// ── Download + extract bin ────────────────────────────────────────────────
	if needs.repairBin {
		if err := checkDiskSpace(installDir, minFreeBytesForBins); err != nil {
			report(InstallState{Err: err})
			return
		}

		binURL := assetURL(latest, platformBinAsset())
		if binURL == "" {
			report(InstallState{Err: fmt.Errorf("Bin package not found in latest release — please try again later.")})
			return
		}

		report(InstallState{Status: "Downloading game binaries...", Progress: 0.70})
		binTmp, err := downloadWithProgressCallback(binURL, func(downloaded, total int64) {
			if total > 0 {
				pct := float64(downloaded) / float64(total)
				report(InstallState{
					Status:   fmt.Sprintf("Downloading game binaries... %.0f%%", pct*100),
					Progress: 0.70 + pct*0.15,
				})
			}
		})
		if err != nil {
			report(InstallState{Err: fmt.Errorf(
				"Download interrupted: %v\n\nRun the installer again to resume.", err)})
			return
		}
		defer os.Remove(binTmp)

		binDir := filepath.Join(installDir, "bin", "x64")
		if err := os.MkdirAll(binDir, 0755); err != nil {
			report(InstallState{Err: fmt.Errorf("Could not create bin directory: %v", err)})
			return
		}

		// Verify bin integrity before extracting
		if expectedHash := fetchAssetChecksum(latest, platformBinAsset()); expectedHash != "" {
			if err := verifyDownloadChecksum(binTmp, expectedHash); err != nil {
				os.Remove(binTmp)
				report(InstallState{Err: fmt.Errorf("Bin download integrity check failed: %v", err)})
				return
			}
		}

		report(InstallState{Status: "Extracting game binaries...", Progress: 0.85})
		if err := extractZip(binTmp, binDir); err != nil {
			report(InstallState{Err: fmt.Errorf("Failed to extract game binaries: %v", err)})
			return
		}
		if runtime.GOOS != "windows" {
			chmodSo(binDir)
		}
		if err := validateBinDir(binDir); err != nil {
			report(InstallState{Err: fmt.Errorf(
				"Binaries failed validation: %v\n\nRun the installer again to retry.", err)})
			return
		}
	}

	// ── Copy updater into install location ────────────────────────────────────
	report(InstallState{Status: "Installing updater...", Progress: 0.88})
	exe, _ := os.Executable()
	binDir := filepath.Join(installDir, "bin", "x64")
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

	finalize(report, steamPath, updaterDest)

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

	if err := validateBinDir(filepath.Join(installDir, "bin", "x64")); err != nil {
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

func finalize(report func(InstallState), steamPath, updaterPath string) {
	report(InstallState{Status: "Configuring Steam launch option...", Progress: 0.92})
	if err := closeSteam(); err != nil {
		termWarn("Could not close Steam: %v", err)
	}

	launchOption := fmt.Sprintf(`"%s" %%command%%`, updaterPath)
	if err := setLaunchOption(steamPath, updaterPath); err != nil {
		termWarn("Could not set launch option automatically: %v", err)
		report(InstallState{
			Status:       "Steam launch option could not be set automatically — see below.",
			Progress:     0.95,
			ManualLaunch: launchOption,
		})
	}

	report(InstallState{Status: "Relaunching Steam...", Progress: 0.97})
	relaunchSteam(steamPath)

	report(InstallState{
		Status:   "Installation complete!\nTF2 Vintage will appear in your Steam library shortly.",
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
