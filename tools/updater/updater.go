package main

import (
	"archive/zip"
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
	localCommit := readField(filepath.Join(modDir, "bin", "x64", "version-bin.txt"), "commit")

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
	binDir := filepath.Join(modDir, "bin", "x64")
	if err := extractZip(tmp, binDir); err != nil {
		return err
	}
	// On Linux, .zip does not preserve execute permissions — fix .so files
	if runtime.GOOS != "windows" {
		chmodSo(binDir)
	}
	return nil
}

// ── Base update ───────────────────────────────────────────────────────────────

func updateBase(modDir string, latest *ghRelease) error {
	manifestURL := assetURL(latest, "base-manifest.json")
	if manifestURL == "" {
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
		url := assetURL(latest, "tf2vintage-base.zip")
		if url == "" {
			return fmt.Errorf("full base asset not found in release")
		}
		tmp, err := downloadWithProgress(url)
		if err != nil {
			return err
		}
		defer os.Remove(tmp)

		fmt.Println("Extracting base...")
		if err := extractZip(tmp, modDir); err != nil {
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
		fmt.Println("Patch chain unavailable — falling back to full base download...")
		return fullBaseDownload(modDir, latest, remoteManifest, localManifestPath)
	}

	fmt.Printf("Applying %d patch(es)...\n", len(chain))
	for i, release := range chain {
		fmt.Printf("[%d/%d] Applying patch %s\n", i+1, len(chain), release.TagName)
		if err := applyPatch(modDir, release); err != nil {
			fmt.Printf("Patch %s failed (%v) — falling back to full base download...\n", release.TagName, err)
			return fullBaseDownload(modDir, latest, remoteManifest, localManifestPath)
		}
	}

	saveManifest(localManifestPath, remoteManifest)
	fmt.Println("Base updated via patch chain.")
	return nil
}

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

func applyPatch(modDir string, release *ghRelease) error {
	url := assetURL(release, "base-patch.zip")
	if url == "" {
		return nil
	}

	tmp, err := downloadWithProgress(url)
	if err != nil {
		return err
	}
	defer os.Remove(tmp)

	return extractZip(tmp, modDir)
}

func fullBaseDownload(modDir string, latest *ghRelease, manifest *BaseManifest, localManifestPath string) error {
	url := assetURL(latest, "tf2vintage-base.zip")
	if url == "" {
		return fmt.Errorf("full base asset not found in release")
	}
	tmp, err := downloadWithProgress(url)
	if err != nil {
		return err
	}
	defer os.Remove(tmp)

	fmt.Println("Extracting base...")
	if err := extractZip(tmp, modDir); err != nil {
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

// extractZip extracts a .zip archive into destDir, stripping the first
// path component (e.g. "tf2vintage/cfg/foo.cfg" → destDir/cfg/foo.cfg)
func extractZip(src, destDir string) error {
	r, err := zip.OpenReader(src)
	if err != nil {
		return err
	}
	defer r.Close()

	for _, f := range r.File {
		// Strip leading path component
		parts := strings.SplitN(f.Name, "/", 2)
		relPath := f.Name
		if len(parts) == 2 {
			relPath = parts[1]
		}
		if relPath == "" {
			continue
		}

		target := filepath.Join(destDir, filepath.FromSlash(relPath))

		if f.FileInfo().IsDir() {
			os.MkdirAll(target, 0755)
			continue
		}

		os.MkdirAll(filepath.Dir(target), 0755)
		out, err := os.OpenFile(target, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, f.Mode())
		if err != nil {
			return err
		}
		rc, err := f.Open()
		if err != nil {
			out.Close()
			return err
		}
		_, cerr := io.Copy(out, rc)
		rc.Close()
		out.Close()
		if cerr != nil {
			return cerr
		}
	}
	return nil
}

// ── Misc helpers ──────────────────────────────────────────────────────────────

// chmodSo restores execute permissions on .so files after zip extraction.
// zip does not preserve Unix file permissions, so this is required on Linux.
func chmodSo(dir string) {
	filepath.WalkDir(dir, func(path string, d os.DirEntry, err error) error {
		if err != nil || d.IsDir() {
			return err
		}
		if strings.HasSuffix(path, ".so") {
			os.Chmod(path, 0755)
		}
		return nil
	})
}

func platformBinAsset() string {
	return "tf2vintage-bin.zip"
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
