package main

import (
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"time"
)

// errUpToDate is returned by updateBin/updateBase/updateSymbols when the
// installed version already matches the latest release. Nothing is written
// to the staging directory in this case, so the caller must not atomicSwapDir.
var errUpToDate = errors.New("already up to date")

func runUpdateMode(exe string) {
	// bin/<platform> → bin → tf2vintage
	modDir := modDirFromExe(exe)
	liveBinDir := platformBinDir(modDir)

	// Staging root: sits next to the mod dir so it's on the same filesystem,
	// which is required for os.Rename to work atomically.
	stagingRoot := modDir + ".staging"
	stagingBinDir := filepath.Join(stagingRoot, "bin", binDirName())
	stagingModDir := filepath.Join(stagingRoot, "mod")

	// ── Handle config flags ───────────────────────────────────────────────────
	// These read/write config from the live bin dir and exit immediately —
	// they don't participate in the update flow at all and don't need a lock.
	if len(os.Args) > 1 {
		switch os.Args[1] {
		case "--enable-symbols":
			cfg := loadConfig(liveBinDir)
			cfg.DownloadSymbols = true
			if err := saveConfig(liveBinDir, cfg); err != nil {
				termFatal("Could not save config: %v", err)
			}
			fmt.Println("Symbol downloads enabled.")
			fmt.Println("Debug symbols will be downloaded on the next update.")
			fmt.Println("To disable: run tf2vintage-updater --disable-symbols")
			termPause()
			os.Exit(0)
		case "--disable-symbols":
			cfg := loadConfig(liveBinDir)
			cfg.DownloadSymbols = false
			if err := saveConfig(liveBinDir, cfg); err != nil {
				termFatal("Could not save config: %v", err)
			}
			fmt.Println("Symbol downloads disabled.")
			termPause()
			os.Exit(0)
		case "--enable-nightly":
			cfg := loadConfig(liveBinDir)
			cfg.CheckNightly = true
			if err := saveConfig(liveBinDir, cfg); err != nil {
				termFatal("Could not save config: %v", err)
			}
			fmt.Println("Nightly updates enabled.")
			fmt.Println("The updater will check for nightly pre-releases on each launch.")
			fmt.Println("Note: nightlies contain only binary updates, not game asset changes.")
			fmt.Println("To disable: run tf2vintage-updater --disable-nightly")
			termPause()
			os.Exit(0)
		case "--disable-nightly":
			cfg := loadConfig(liveBinDir)
			cfg.CheckNightly = false
			if err := saveConfig(liveBinDir, cfg); err != nil {
				termFatal("Could not save config: %v", err)
			}
			fmt.Println("Nightly updates disabled.")
			fmt.Println("The updater will only install stable releases.")
			termPause()
			os.Exit(0)
		}
	}

	steamArgs := os.Args[1:]
	standaloneMode := len(steamArgs) == 0

	termPrintBanner()

	// ── Acquire lockfile (against live bin dir) ───────────────────────────────
	release, err := acquireLock(liveBinDir)
	if err != nil {
		termFatal("%v", err)
	}
	defer release()

	// ── Set up staging directory (after lock, so only one instance uses it) ──
	os.RemoveAll(stagingRoot)
	if err := os.MkdirAll(stagingBinDir, 0755); err != nil {
		termFatal("Could not create staging directory: %v", err)
	}
	if err := os.MkdirAll(stagingModDir, 0755); err != nil {
		termFatal("Could not create staging directory: %v", err)
	}

	// ── Load user config from live location ──────────────────────────────────
	cfg := loadConfig(liveBinDir)

	// ── Check disk space (staging is beside modDir, same drive) ──────────────
	if err := checkDiskSpace(modDir, minFreeBytesForBins+minFreeBytesForBase); err != nil {
		termWarn("Disk space check failed: %v — proceeding anyway", err)
	}

	// ── Fetch latest stable release (cached) ─────────────────────────────────
	termPrint("Checking for updates...")
	latest, err := fetchLatestRelease()
	if err != nil {
		termWarn("Could not check for updates (%v) — launching with current version.", err)
		os.RemoveAll(stagingRoot)
		launchIfNotStandalone(standaloneMode, steamArgs)
		return
	}

	// ── Verify own integrity against published checksum ───────────────────────
	if err := verifyUpdaterChecksum(latest); err != nil {
		termFatal("Integrity check failed: %v", err)
	}

	// ── Optionally fetch the nightly pre-release ──────────────────────────────
	// When enabled, treat the nightly as the bin source if it is newer than
	// both the latest stable release and the currently installed build.
	// Base assets (maps, materials, etc.) are never updated from a nightly —
	// those only change in stable releases.
	var nightlyRelease *ghRelease
	if cfg.CheckNightly {
		if n, err := fetchNightlyRelease(); err != nil {
			termWarn("Could not check for nightly (%v) — using stable release.", err)
		} else {
			nightlyRelease = n
		}
	}

	// ── Download all updates into staging ────────────────────────────────────
	binUpdated := false
	baseUpdated := false

	// Prefer the nightly for bin if it is newer than what's installed.
	// Fall back to the stable release if the nightly is absent or already current.
	binSource := latest
	if nightlyRelease != nil {
		switch err := updateNightly(liveBinDir, stagingBinDir, nightlyRelease); {
		case err == nil:
			binUpdated = true
			binSource = nil // bin already staged from nightly; skip stable bin update
		case errors.Is(err, errUpToDate):
			// nightly is current; still check stable below in case a new stable
			// has arrived since the nightly was built
		default:
			termWarn("Nightly bin update failed: %v — falling back to stable.", err)
		}
	}

	if binSource != nil {
		switch err := updateBin(liveBinDir, stagingBinDir, binSource); {
		case err == nil:
			binUpdated = true
		case errors.Is(err, errUpToDate):
			// nothing staged — do not swap
		default:
			termWarn("Bin update failed: %v", err)
		}
	}

	switch err := updateBase(modDir, stagingModDir, latest); {
	case err == nil:
		baseUpdated = true
	case errors.Is(err, errUpToDate):
		// nothing staged — do not swap
	default:
		termWarn("Base update failed: %v", err)
	}

	if cfg.DownloadSymbols {
		// liveBinDir for version check (persists across runs), stagingBinDir for extraction
		switch err := updateSymbols(liveBinDir, stagingBinDir, latest); {
		case err == nil:
			binUpdated = true // symbols were staged alongside bin — swap both together
		case errors.Is(err, errUpToDate):
			// nothing staged — do not swap
		default:
			termWarn("Symbol update failed: %v", err)
		}
	}

	// ── Atomic swap: move staging into place ──────────────────────────────────
	// Only swap the subtrees that were actually updated, so a bin failure
	// doesn't roll back a successful base update.
	if binUpdated {
		fmt.Println("Applying bin update...")
		if err := atomicSwapDir(liveBinDir, stagingBinDir); err != nil {
			termFatal("Bin atomic swap failed: %v", err)
		}
		fmt.Println("Binaries updated.")
	}

	if baseUpdated {
		fmt.Println("Applying base update...")
		if err := atomicSwapDir(modDir, stagingModDir); err != nil {
			termFatal("Base atomic swap failed: %v", err)
		}
		fmt.Println("Base updated.")
	}

	// Clean up staging root (now empty)
	os.RemoveAll(stagingRoot)

	fmt.Println()
	if standaloneMode {
		termPause()
		return
	}
	fmt.Println("Launching TF2 Vintage in 5 seconds...")
	time.Sleep(5 * time.Second)
	launchGame(steamArgs)
}

// ── Nightly bin update ────────────────────────────────────────────────────────

// updateNightly checks the nightly-version.txt asset in the nightly pre-release
// against the locally installed version-bin.txt. It downloads and stages
// tf2vintage-bin.zip only when the nightly build is newer.
//
// Version comparison uses the "short" field (9-char commit SHA prefix):
//
//	local  version-bin.txt  → short=<sha>
//	remote nightly-version.txt → short=<sha>
//
// Returns nil if the nightly was staged, errUpToDate if already current.
func updateNightly(liveBinDir, stagingBinDir string, nightly *ghRelease) error {
	// Fetch the tiny nightly-version.txt from the pre-release to compare
	// without downloading the full bin zip.
	versionURL := assetURL(nightly, "nightly-version.txt")
	if versionURL == "" {
		// Nightly exists but has no version file — malformed release, skip it.
		return fmt.Errorf("nightly release has no nightly-version.txt asset")
	}

	remoteShort, err := fetchRemoteField(versionURL, "short")
	if err != nil {
		return fmt.Errorf("could not read nightly version: %v", err)
	}
	if remoteShort == "" {
		return fmt.Errorf("nightly-version.txt missing 'short' field")
	}

	localShort := readField(filepath.Join(liveBinDir, "version-bin.txt"), "short")

	if localShort != "" && localShort == remoteShort {
		fmt.Println("Nightly binaries are up to date.")
		return errUpToDate
	}

	fmt.Printf("Nightly bin update: %s → %s\n", shortOrNone(localShort), remoteShort)

	url := assetURL(nightly, platformBinAsset())
	if url == "" {
		return fmt.Errorf("nightly bin asset not found in pre-release")
	}

	fmt.Printf("Downloading nightly %s...\n", platformBinAsset())
	tmp, err := downloadWithProgress(url)
	if err != nil {
		return err
	}

	if expectedHash := fetchAssetChecksum(nightly, platformBinAsset()); expectedHash != "" {
		if err := verifyDownloadChecksum(tmp, expectedHash); err != nil {
			os.Remove(tmp)
			return fmt.Errorf("nightly bin integrity check failed: %v", err)
		}
	}

	fmt.Println("Extracting nightly binaries to staging...")
	stagingRoot := filepath.Dir(filepath.Dir(stagingBinDir))
	if err := extractZipRaw(tmp, stagingRoot); err != nil {
		os.Remove(tmp)
		return fmt.Errorf("nightly bin extraction failed: %v", err)
	}
	os.Remove(tmp)

	if runtime.GOOS != "windows" {
		chmodSo(stagingBinDir)
	}

	if err := validateBinDir(stagingBinDir); err != nil {
		return fmt.Errorf("nightly bin staging failed validation: %v", err)
	}

	return nil
}

// ── Stable bin update ─────────────────────────────────────────────────────────

// updateBin downloads the bin package into stagingBinDir.
// liveBinDir is read-only here — used only to check the current version.
// Returns nil only if a new version was downloaded and is ready to swap in.
// Returns errUpToDate (no staging writes) if already up to date.
func updateBin(liveBinDir, stagingBinDir string, latest *ghRelease) error {
	remoteTag := latest.TagName
	localCommit := readField(filepath.Join(liveBinDir, "version-bin.txt"), "commit")

	if localCommit != "" && !validateCommitSHA(localCommit) {
		termWarn("version-bin.txt has invalid commit SHA — forcing bin update")
		localCommit = ""
	}

	if localCommit != "" && remoteTag == "build-"+localCommit {
		fmt.Println("Binaries are up to date.")
		return errUpToDate
	}

	fmt.Printf("Bin update: %s → %s\n", shortOrNone(localCommit), remoteTag)

	url := assetURL(latest, platformBinAsset())
	if url == "" {
		return fmt.Errorf("bin asset not found in release")
	}

	fmt.Printf("Downloading %s...\n", platformBinAsset())
	tmp, err := downloadWithProgress(url)
	if err != nil {
		return err
	}

	// Verify integrity before extracting into staging
	if expectedHash := fetchAssetChecksum(latest, platformBinAsset()); expectedHash != "" {
		if err := verifyDownloadChecksum(tmp, expectedHash); err != nil {
			os.Remove(tmp)
			return fmt.Errorf("bin download integrity check failed: %v", err)
		}
	}

	fmt.Println("Extracting binaries to staging...")
	// tf2vintage-bin.zip contains "bin/x64/..." or "bin/linux64/..." paths verbatim,
	// so extract into the staging root (two levels above stagingBinDir) to let the
	// zip reconstruct the full bin/<platform>/ subtree there.
	stagingRoot := filepath.Dir(filepath.Dir(stagingBinDir))
	if err := extractZipRaw(tmp, stagingRoot); err != nil {
		os.Remove(tmp)
		return fmt.Errorf("bin extraction failed: %v", err)
	}
	os.Remove(tmp)

	if runtime.GOOS != "windows" {
		chmodSo(stagingBinDir)
	}

	// Validate staging before we commit to swapping it in
	if err := validateBinDir(stagingBinDir); err != nil {
		return fmt.Errorf("bin staging failed validation: %v", err)
	}

	return nil
}

// ── Base update ───────────────────────────────────────────────────────────────

// updateBase downloads/patches base assets into stagingModDir.
// liveModDir is read-only — used only to check current manifest.
// Returns nil only if new content is ready in stagingModDir.
// Returns errUpToDate (no staging writes) if already up to date.
func updateBase(liveModDir, stagingModDir string, latest *ghRelease) error {
	manifestURL := assetURL(latest, "base-manifest.json")
	if manifestURL == "" {
		return errUpToDate
	}

	remoteManifest, err := fetchManifest(manifestURL)
	if err != nil {
		return fmt.Errorf("could not fetch remote manifest: %v", err)
	}

	localManifestPath := filepath.Join(liveModDir, "base-manifest.json")
	localManifest := loadLocalManifest(localManifestPath)

	if localManifest == nil {
		fmt.Println("No base install found — downloading full base (this may take a while)...")
		return fullBaseDownload(stagingModDir, latest, remoteManifest,
			filepath.Join(stagingModDir, "base-manifest.json"))
	}

	if localManifest.Tag == remoteManifest.Tag {
		fmt.Println("Base is up to date.")
		return errUpToDate
	}

	fmt.Printf("Base update: %s → %s\n", localManifest.Tag, remoteManifest.Tag)

	chain, err := buildPatchChain(localManifest.Tag, latest)
	if err != nil {
		fmt.Printf("Patch chain unavailable (%v) — falling back to full base download...\n", err)
		return fullBaseDownload(stagingModDir, latest, remoteManifest,
			filepath.Join(stagingModDir, "base-manifest.json"))
	}

	// For patch application we need the current files as a base.
	// Copy live mod dir into staging first, then apply patches on top.
	fmt.Println("Seeding staging from current install...")
	if err := copyDir(liveModDir, stagingModDir); err != nil {
		fmt.Printf("Could not seed staging (%v) — falling back to full base download...\n", err)
		os.RemoveAll(stagingModDir)
		os.MkdirAll(stagingModDir, 0755)
		return fullBaseDownload(stagingModDir, latest, remoteManifest,
			filepath.Join(stagingModDir, "base-manifest.json"))
	}

	fmt.Printf("Applying %d patch(es)...\n", len(chain))
	for i, release := range chain {
		fmt.Printf("[%d/%d] Applying patch %s\n", i+1, len(chain), release.TagName)
		if err := applyPatch(stagingModDir, release); err != nil {
			fmt.Printf("Patch %s failed (%v) — falling back to full base download...\n", release.TagName, err)
			os.RemoveAll(stagingModDir)
			os.MkdirAll(stagingModDir, 0755)
			return fullBaseDownload(stagingModDir, latest, remoteManifest,
				filepath.Join(stagingModDir, "base-manifest.json"))
		}
	}

	saveManifest(filepath.Join(stagingModDir, "base-manifest.json"), remoteManifest)
	fmt.Println("Base patched in staging.")
	return nil
}

// buildPatchChain walks backwards through releases using cached manifests,
// building the ordered list of patches to apply from localTag to latest.
func buildPatchChain(localTag string, latest *ghRelease) ([]*ghRelease, error) {
	const maxChain = 20
	var chain []*ghRelease
	current := latest

	for i := 0; i < maxChain; i++ {
		manifest, err := fetchReleaseManifest(current)
		if err != nil {
			return nil, fmt.Errorf("could not fetch manifest for %s: %v", current.TagName, err)
		}

		chain = append([]*ghRelease{current}, chain...)

		if manifest.PrevTag == localTag {
			return chain, nil
		}
		if manifest.PrevTag == "" {
			return nil, fmt.Errorf("patch chain does not reach local tag %s", localTag)
		}

		prev, err := fetchRelease(manifest.PrevTag)
		if err != nil {
			return nil, fmt.Errorf("could not fetch release %s: %v", manifest.PrevTag, err)
		}
		current = prev
	}

	return nil, fmt.Errorf("patch chain exceeded %d steps", maxChain)
}

func applyPatch(destDir string, release *ghRelease) error {
	url := assetURL(release, "base-patch.zip")
	if url == "" {
		return nil
	}
	tmp, err := downloadWithProgress(url)
	if err != nil {
		return err
	}
	defer os.Remove(tmp)
	return extractZip(tmp, destDir)
}

func fullBaseDownload(destDir string, latest *ghRelease, manifest *BaseManifest, localManifestPath string) error {
	if err := checkDiskSpace(destDir, minFreeBytesForBase); err != nil {
		return err
	}

	url := assetURL(latest, "tf2vintage-base.zip")
	if url == "" {
		return fmt.Errorf("full base asset not found in release")
	}
	tmp, err := downloadWithProgress(url)
	if err != nil {
		return err
	}

	if expectedHash := fetchAssetChecksum(latest, "tf2vintage-base.zip"); expectedHash != "" {
		if err := verifyDownloadChecksum(tmp, expectedHash); err != nil {
			os.Remove(tmp)
			return fmt.Errorf("base download integrity check failed: %v", err)
		}
	}

	fmt.Println("Extracting base to staging...")
	if err := extractZip(tmp, destDir); err != nil {
		os.Remove(tmp)
		return err
	}
	os.Remove(tmp)
	saveManifest(localManifestPath, manifest)
	fmt.Println("Base staged.")
	return nil
}
