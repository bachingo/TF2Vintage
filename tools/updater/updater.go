package main

import (
	"archive/tar"
	"compress/gzip"
	"crypto/md5"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"runtime"
	"strings"
	"time"
)

const (
	repoOwner = "TF2V"
	repoName  = "TF2Vintage"
)

// ── GitHub API types ──────────────────────────────────────────────────────────

type ghRelease struct {
	TagName string    `json:"tag_name"`
	Assets  []ghAsset `json:"assets"`
}

type ghAsset struct {
	Name               string `json:"name"`
	BrowserDownloadURL string `json:"browser_download_url"`
}

// ── Manifest types ────────────────────────────────────────────────────────────

// BaseManifest describes every file in the base asset tree.
// PrevTag is the release tag this build was diffed against — used to
// walk the patch chain when the player is multiple versions behind.
type BaseManifest struct {
	Tag     string            `json:"tag"`
	PrevTag string            `json:"prev_tag"`
	Files   map[string]string `json:"files"` // relative path → MD5
}

// ── Entry point ───────────────────────────────────────────────────────────────

func main() {
	steamArgs := os.Args[1:]
	standaloneMode := len(steamArgs) == 0

	printBanner()

	exe, err := os.Executable()
	if err != nil {
		fatal("Could not locate updater binary: %v", err)
	}
	modDir := filepath.Dir(exe)

	// ── 1. Fetch latest release ───────────────────────────────────────────────
	print("Checking for updates... ")
	latest, err := fetchLatestRelease()
	if err != nil {
		warn("Could not reach GitHub (%v) — skipping update check.", err)
		launchIfNotStandalone(standaloneMode, steamArgs)
		return
	}

	// ── 2. Bin update ─────────────────────────────────────────────────────────
	if err := updateBin(modDir, latest); err != nil {
		warn("Bin update failed: %v", err)
	}

	// ── 3. Base update (manifest-driven delta) ────────────────────────────────
	if err := updateBase(modDir, latest); err != nil {
		warn("Base update failed: %v", err)
	}

	fmt.Println()
	if standaloneMode {
		pause()
		return
	}
	fmt.Println("Launching TF2 Vintage in 3 seconds...")
	time.Sleep(3 * time.Second)
	launchGame(steamArgs)
}

// ── Bin update ────────────────────────────────────────────────────────────────

func updateBin(modDir string, latest *ghRelease) error {
	remoteCommit := latest.TagName
	localCommit := readField(filepath.Join(modDir, "bin", "version-bin.txt"), "commit")

	if remoteCommit == localCommit {
		fmt.Println("Binaries are up to date.")
		return nil
	}

	fmt.Printf("Bin update: %s → %s\n", shortOrNone(localCommit), shortOrNone(remoteCommit))

	asset := platformBinAsset()
	url := assetURL(latest, asset)
	if url == "" {
		return fmt.Errorf("asset '%s' not found in release", asset)
	}

	fmt.Printf("Downloading %s...\n", asset)
	tmp, err := downloadWithProgress(url)
	if err != nil {
		return err
	}
	defer os.Remove(tmp)

	fmt.Println("Extracting binaries...")
	return extractTarGz(tmp, filepath.Join(modDir, "bin"))
}

// ── Base update ───────────────────────────────────────────────────────────────

func updateBase(modDir string, latest *ghRelease) error {
	// Fetch the latest manifest
	manifestURL := assetURL(latest, "base-manifest.json")
	if manifestURL == "" {
		// No base asset in this release — nothing to do
		return nil
	}

	remoteManifest, err := fetchManifest(manifestURL)
	if err != nil {
		return fmt.Errorf("could not fetch remote manifest: %v", err)
	}

	localManifestPath := filepath.Join(modDir, "base-manifest.json")
	localManifest := loadLocalManifest(localManifestPath)

	if localManifest == nil {
		// ── Fresh install: download full base ─────────────────────────────────
		fmt.Println("No base install found — downloading full base (this may take a while)...")
		url := assetURL(latest, "tf2vintage-base.tar.gz")
		if url == "" {
			return fmt.Errorf("full base asset not found in release")
		}
		tmp, err := downloadWithProgress(url)
		if err != nil {
			return err
		}
		defer os.Remove(tmp)

		fmt.Println("Extracting base...")
		if err := extractTarGz(tmp, modDir); err != nil {
			return err
		}
		saveManifest(localManifestPath, remoteManifest)
		fmt.Println("Base installed.")
		return nil
	}

	if localManifest.Tag == remoteManifest.Tag {
		fmt.Println("Base is up to date.")
		return nil
	}

	// ── Delta update: build patch chain from local tag to latest ─────────────
	fmt.Printf("Base update: %s → %s\n", localManifest.Tag, remoteManifest.Tag)

	chain, err := buildPatchChain(localManifest.Tag, latest)
	if err != nil {
		// Chain is broken or too long — fall back to full base
		fmt.Println("Patch chain unavailable — falling back to full base download...")
		return fullBaseDownload(modDir, latest, remoteManifest, localManifestPath)
	}

	fmt.Printf("Applying %d patch(es)...\n", len(chain))
	for i, release := range chain {
		fmt.Printf("[%d/%d] Applying patch %s\n", i+1, len(chain), release.TagName)
		if err := applyPatch(modDir, release); err != nil {
			// If any patch in the chain fails, fall back to full base
			fmt.Printf("Patch %s failed (%v) — falling back to full base download...\n", release.TagName, err)
			return fullBaseDownload(modDir, latest, remoteManifest, localManifestPath)
		}
	}

	saveManifest(localManifestPath, remoteManifest)
	fmt.Println("Base updated via patch chain.")
	return nil
}

// buildPatchChain walks GitHub releases backwards from latest until it finds
// a release whose PrevTag matches the player's current local tag.
// Returns releases in oldest-first order (correct application order).
func buildPatchChain(localTag string, latest *ghRelease) ([]*ghRelease, error) {
	const maxChain = 20

	var chain []*ghRelease
	current := latest

	for i := 0; i < maxChain; i++ {
		manifest, err := fetchReleaseManifest(current)
		if err != nil {
			return nil, fmt.Errorf("could not fetch manifest for %s: %v", current.TagName, err)
		}

		chain = append([]*ghRelease{current}, chain...) // prepend — build oldest-first

		if manifest.PrevTag == localTag {
			// Found the link back to the player's current version
			return chain, nil
		}
		if manifest.PrevTag == "" {
			// Reached the beginning of release history — chain is broken
			return nil, fmt.Errorf("patch chain does not reach local tag %s", localTag)
		}

		// Fetch the previous release and continue walking back
		prev, err := fetchRelease(manifest.PrevTag)
		if err != nil {
			return nil, fmt.Errorf("could not fetch release %s: %v", manifest.PrevTag, err)
		}
		current = prev
	}

	return nil, fmt.Errorf("patch chain exceeded %d steps", maxChain)
}

func applyPatch(modDir string, release *ghRelease) error {
	url := assetURL(release, "base-patch.tar.gz")
	if url == "" {
		// This release had no base changes — skip silently
		return nil
	}

	tmp, err := downloadWithProgress(url)
	if err != nil {
		return err
	}
	defer os.Remove(tmp)

	return extractTarGz(tmp, modDir)
}

func fullBaseDownload(modDir string, latest *ghRelease, manifest *BaseManifest, localManifestPath string) error {
	url := assetURL(latest, "tf2vintage-base.tar.gz")
	if url == "" {
		return fmt.Errorf("full base asset not found in release")
	}
	tmp, err := downloadWithProgress(url)
	if err != nil {
		return err
	}
	defer os.Remove(tmp)

	fmt.Println("Extracting base...")
	if err := extractTarGz(tmp, modDir); err != nil {
		return err
	}
	saveManifest(localManifestPath, manifest)
	fmt.Println("Base updated (full download).")
	return nil
}

// ── Manifest helpers ──────────────────────────────────────────────────────────

func fetchReleaseManifest(r *ghRelease) (*BaseManifest, error) {
	url := assetURL(r, "base-manifest.json")
	if url == "" {
		return nil, fmt.Errorf("no base-manifest.json in release %s", r.TagName)
	}
	return fetchManifest(url)
}

func fetchManifest(url string) (*BaseManifest, error) {
	resp, err := http.Get(url)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()
	var m BaseManifest
	if err := json.NewDecoder(resp.Body).Decode(&m); err != nil {
		return nil, err
	}
	return &m, nil
}

func loadLocalManifest(path string) *BaseManifest {
	b, err := os.ReadFile(path)
	if err != nil {
		return nil
	}
	var m BaseManifest
	if err := json.Unmarshal(b, &m); err != nil {
		return nil
	}
	return &m
}

func saveManifest(path string, m *BaseManifest) {
	b, err := json.MarshalIndent(m, "", "  ")
	if err != nil {
		return
	}
	os.WriteFile(path, b, 0644)
}

// md5File returns the hex MD5 of a file (used if we ever need local diffing)
func md5File(path string) (string, error) {
	f, err := os.Open(path)
	if err != nil {
		return "", err
	}
	defer f.Close()
	h := md5.New()
	if _, err := io.Copy(h, f); err != nil {
		return "", err
	}
	return hex.EncodeToString(h.Sum(nil)), nil
}

// ── GitHub API ────────────────────────────────────────────────────────────────

func fetchLatestRelease() (*ghRelease, error) {
	return fetchReleaseByURL(fmt.Sprintf(
		"https://api.github.com/repos/%s/%s/releases/latest",
		repoOwner, repoName,
	))
}

func fetchRelease(tag string) (*ghRelease, error) {
	return fetchReleaseByURL(fmt.Sprintf(
		"https://api.github.com/repos/%s/%s/releases/tags/%s",
		repoOwner, repoName, tag,
	))
}

func fetchReleaseByURL(url string) (*ghRelease, error) {
	resp, err := http.Get(url)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()
	var r ghRelease
	if err := json.NewDecoder(resp.Body).Decode(&r); err != nil {
		return nil, err
	}
	return &r, nil
}

func assetURL(r *ghRelease, name string) string {
	for _, a := range r.Assets {
		if a.Name == name {
			return a.BrowserDownloadURL
		}
	}
	return ""
}

// ── Download ──────────────────────────────────────────────────────────────────

func downloadWithProgress(url string) (string, error) {
	resp, err := http.Get(url)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()

	f, err := os.CreateTemp("", "tf2v-update-*")
	if err != nil {
		return "", err
	}
	defer f.Close()

	total := resp.ContentLength
	var downloaded int64
	buf := make([]byte, 32*1024)
	lastPrint := time.Now()

	for {
		n, err := resp.Body.Read(buf)
		if n > 0 {
			if _, werr := f.Write(buf[:n]); werr != nil {
				os.Remove(f.Name())
				return "", werr
			}
			downloaded += int64(n)
			if time.Since(lastPrint) > 250*time.Millisecond {
				printProgress(downloaded, total)
				lastPrint = time.Now()
			}
		}
		if err == io.EOF {
			break
		}
		if err != nil {
			os.Remove(f.Name())
			return "", err
		}
	}
	printProgress(downloaded, total)
	fmt.Println()
	return f.Name(), nil
}

func printProgress(downloaded, total int64) {
	if total > 0 {
		pct := float64(downloaded) / float64(total) * 100
		fmt.Printf("\r  %.1f MB / %.1f MB (%.0f%%)   ",
			float64(downloaded)/1024/1024,
			float64(total)/1024/1024,
			pct,
		)
	} else {
		fmt.Printf("\r  %.1f MB downloaded   ", float64(downloaded)/1024/1024)
	}
}

// ── Extraction ────────────────────────────────────────────────────────────────

func extractTarGz(src, destDir string) error {
	f, err := os.Open(src)
	if err != nil {
		return err
	}
	defer f.Close()

	gz, err := gzip.NewReader(f)
	if err != nil {
		return err
	}
	defer gz.Close()

	tr := tar.NewReader(gz)
	for {
		hdr, err := tr.Next()
		if err == io.EOF {
			break
		}
		if err != nil {
			return err
		}

		parts := strings.SplitN(hdr.Name, "/", 2)
		relPath := hdr.Name
		if len(parts) == 2 {
			relPath = parts[1]
		}
		if relPath == "" {
			continue
		}

		target := filepath.Join(destDir, filepath.FromSlash(relPath))
		switch hdr.Typeflag {
		case tar.TypeDir:
			os.MkdirAll(target, 0755)
		case tar.TypeReg:
			os.MkdirAll(filepath.Dir(target), 0755)
			out, err := os.OpenFile(target, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, os.FileMode(hdr.Mode))
			if err != nil {
				return err
			}
			_, cerr := io.Copy(out, tr)
			out.Close()
			if cerr != nil {
				return cerr
			}
		}
	}
	return nil
}

// ── Misc helpers ──────────────────────────────────────────────────────────────

func platformBinAsset() string {
	if runtime.GOOS == "windows" {
		return "tf2vintage-windows-bin.exe"
	}
	return "tf2vintage-linux-bin.tar.gz"
}

func readField(path, key string) string {
	b, err := os.ReadFile(path)
	if err != nil {
		return ""
	}
	prefix := key + "="
	for _, line := range strings.Split(string(b), "\n") {
		line = strings.TrimSpace(line)
		if strings.HasPrefix(line, prefix) {
			return strings.TrimSpace(strings.TrimPrefix(line, prefix))
		}
	}
	return ""
}

func shortOrNone(commit string) string {
	if commit == "" {
		return "(none)"
	}
	if len(commit) > 9 {
		return commit[:9]
	}
	return commit
}

func launchGame(steamArgs []string) {
	if len(steamArgs) == 0 {
		return
	}
	cmd := newCmd(steamArgs[0], steamArgs[1:]...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		fatal("Failed to launch game: %v", err)
	}
}

func launchIfNotStandalone(standalone bool, steamArgs []string) {
	if standalone {
		pause()
	} else {
		launchGame(steamArgs)
	}
}

func printBanner() {
	fmt.Println("================================")
	fmt.Println("   TF2 Vintage Updater")
	fmt.Println("================================")
	fmt.Println()
}

func pause() {
	fmt.Println()
	fmt.Print("Press Enter to close...")
	fmt.Scanln()
}

func warn(format string, args ...any) {
	fmt.Fprintf(os.Stderr, "[WARN] "+format+"\n", args...)
}

func fatal(format string, args ...any) {
	fmt.Fprintf(os.Stderr, "[ERROR] "+format+"\n", args...)
	pause()
	os.Exit(1)
}
