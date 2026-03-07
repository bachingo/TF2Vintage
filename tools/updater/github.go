package main

import (
	"encoding/json"
	"fmt"
	"net/http"
	"os"
	"path/filepath"
	"time"
)

const (
	repoOwner       = "TF2V"
	repoName        = "TF2Vintage"
	apiBase         = "https://api.github.com"
	releaseCacheTTL = 60 * time.Minute
	// Nightly is published weekly, so a longer cache is appropriate.
	// 24 h means at most one API call per day even if the updater runs often.
	nightlyCacheTTL = 24 * time.Hour
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

// releaseCache is the on-disk cache entry for a release API response.
type releaseCache struct {
	FetchedAt time.Time  `json:"fetched_at"`
	ETag      string     `json:"etag"`
	Release   *ghRelease `json:"release"`
}

// ── Public API ────────────────────────────────────────────────────────────────

// fetchLatestRelease returns the latest non-pre-release.
// The /releases/latest endpoint never returns pre-releases, so the nightly
// rolling tag is automatically excluded.
func fetchLatestRelease() (*ghRelease, error) {
	cacheDir := releaseCacheDir()
	cachePath := filepath.Join(cacheDir, "latest-release.json")
	return fetchReleaseWithCache(
		fmt.Sprintf("%s/repos/%s/%s/releases/latest", apiBase, repoOwner, repoName),
		cachePath,
		releaseCacheTTL,
	)
}

// fetchNightlyRelease returns the pre-release pinned to the "nightly" tag, or
// nil if no nightly has been published yet. The caller must check for nil before
// use; a missing nightly is not an error.
func fetchNightlyRelease() (*ghRelease, error) {
	cacheDir := releaseCacheDir()
	cachePath := filepath.Join(cacheDir, "nightly-release.json")
	r, err := fetchReleaseWithCache(
		fmt.Sprintf("%s/repos/%s/%s/releases/tags/nightly", apiBase, repoOwner, repoName),
		cachePath,
		nightlyCacheTTL,
	)
	if err != nil {
		// A 404 means no nightly has been published yet — not an error for the
		// caller, just nothing to do.
		return nil, nil //nolint:nilerr
	}
	return r, nil
}

func fetchRelease(tag string) (*ghRelease, error) {
	cacheDir := releaseCacheDir()
	cachePath := filepath.Join(cacheDir, "release-"+sanitizeFilename(tag)+".json")
	return fetchReleaseWithCache(
		fmt.Sprintf("%s/repos/%s/%s/releases/tags/%s", apiBase, repoOwner, repoName, tag),
		cachePath,
		releaseCacheTTL,
	)
}

func assetURL(r *ghRelease, name string) string {
	for _, a := range r.Assets {
		if a.Name == name {
			return a.BrowserDownloadURL
		}
	}
	return ""
}

// ── Cache-aware fetch ─────────────────────────────────────────────────────────

func fetchReleaseWithCache(url, cachePath string, ttl time.Duration) (*ghRelease, error) {
	os.MkdirAll(filepath.Dir(cachePath), 0755)

	cached := loadReleaseCache(cachePath)

	// If cache is fresh, return it without hitting the API
	if cached != nil && time.Since(cached.FetchedAt) < ttl {
		return cached.Release, nil
	}

	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return nil, err
	}
	req.Header.Set("Accept", "application/vnd.github+json")
	req.Header.Set("X-GitHub-Api-Version", "2022-11-28")

	// Send ETag for conditional request — GitHub returns 304 if nothing changed,
	// which doesn't count against the rate limit
	if cached != nil && cached.ETag != "" {
		req.Header.Set("If-None-Match", cached.ETag)
	}

	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		// Network failure — return stale cache if available rather than failing
		if cached != nil {
			termWarn("GitHub unreachable, using cached release info (%s old)",
				time.Since(cached.FetchedAt).Round(time.Minute))
			return cached.Release, nil
		}
		return nil, fmt.Errorf("could not reach GitHub: %v", err)
	}
	defer resp.Body.Close()

	// 304 Not Modified — cache is still valid, update timestamp
	if resp.StatusCode == http.StatusNotModified && cached != nil {
		cached.FetchedAt = time.Now()
		saveReleaseCache(cachePath, cached)
		return cached.Release, nil
	}

	// Rate limited — back off and return stale cache if available
	if resp.StatusCode == http.StatusForbidden || resp.StatusCode == http.StatusTooManyRequests {
		retryAfter := resp.Header.Get("Retry-After")
		if cached != nil {
			termWarn("GitHub rate limit hit (retry after: %s) — using cached release info", retryAfter)
			return cached.Release, nil
		}
		return nil, fmt.Errorf("GitHub API rate limit exceeded — please try again later (retry after: %s)", retryAfter)
	}

	if resp.StatusCode == http.StatusNotFound {
		return nil, fmt.Errorf("release not found: %s", url)
	}

	if resp.StatusCode != http.StatusOK {
		if cached != nil {
			termWarn("GitHub returned %s — using cached release info", resp.Status)
			return cached.Release, nil
		}
		return nil, fmt.Errorf("GitHub API returned unexpected status: %s", resp.Status)
	}

	var release ghRelease
	if err := json.NewDecoder(resp.Body).Decode(&release); err != nil {
		if cached != nil {
			return cached.Release, nil
		}
		return nil, fmt.Errorf("could not parse GitHub response: %v", err)
	}

	// Save fresh cache entry
	entry := &releaseCache{
		FetchedAt: time.Now(),
		ETag:      resp.Header.Get("ETag"),
		Release:   &release,
	}
	saveReleaseCache(cachePath, entry)

	return &release, nil
}

// ── Cache persistence ─────────────────────────────────────────────────────────

func loadReleaseCache(path string) *releaseCache {
	b, err := os.ReadFile(path)
	if err != nil {
		return nil
	}
	var c releaseCache
	if err := json.Unmarshal(b, &c); err != nil {
		return nil
	}
	return &c
}

func saveReleaseCache(path string, c *releaseCache) {
	b, err := json.MarshalIndent(c, "", "  ")
	if err != nil {
		return
	}
	os.WriteFile(path, b, 0644)
}

func releaseCacheDir() string {
	// Store cache alongside the executable so it follows the install
	exe, err := os.Executable()
	if err != nil {
		return os.TempDir()
	}
	return filepath.Join(filepath.Dir(exe), ".cache")
}

func sanitizeFilename(s string) string {
	out := make([]byte, 0, len(s))
	for i := 0; i < len(s); i++ {
		c := s[i]
		if (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' {
			out = append(out, c)
		} else {
			out = append(out, '_')
		}
	}
	return string(out)
}
