[![Build](https://github.com/TF2V/TF2Vintage/actions/workflows/release.yml/badge.svg)](https://github.com/TF2V/TF2Vintage/actions/workflows/release.yml)

# Team Fortress 2 Vintage

A community-driven restoration of classic Team Fortress 2, built on the Source SDK Multiplayer 2013 base.

Development may be slow at times — consider becoming a contributor!

---

## Downloads

TF2 Vintage is distributed as a single installer. Download the updater for your platform and run it — it handles everything else automatically.

**[→ Latest Release](https://github.com/TF2V/TF2Vintage/releases/latest)**

| File | Platform |
|---|---|
| `tf2vintage-updater.exe` | Windows |
| `tf2vintage-updater` | Linux |

The updater downloads `tf2vintage-full.zip` (complete game + binaries) on first run and creates a desktop shortcut. On subsequent launches it checks for updates and applies only what has changed.

> **Release schedule:** Full releases publish every 84 days (12 weeks). Nightly builds publish every Tuesday if there are new commits — nightlies contain only updated binaries, not game asset changes.

---

## Installation

### Prerequisites

- Steam with **Source SDK Base 2013 Multiplayer** and **Team Fortress 2** installed (both free)

### Windows

1. Download `tf2vintage-updater.exe` from the [latest release](https://github.com/TF2V/TF2Vintage/releases/latest).
2. Run it. The installer will locate Steam, download the game files (~350 MB compressed), and create a **TF2 Vintage** shortcut on your Desktop.
3. Double-click the desktop shortcut to play. It checks for updates automatically on each launch.

### Linux

1. Download `tf2vintage-updater` from the [latest release](https://github.com/TF2V/TF2Vintage/releases/latest).
2. Make it executable and run it:
   ```bash
   chmod +x tf2vintage-updater
   ./tf2vintage-updater
   ```
3. The installer downloads the game files and creates a `tf2vintage.desktop` shortcut on your Desktop. Double-click it to play.

---

## Auto-Updater

TF2 Vintage ships with `tf2vintage-updater` as its launcher. The installer places it in `tf2vintage/bin/<platform>/` and creates a desktop shortcut pointing to it. Every time you launch TF2 Vintage through that shortcut, the updater runs first — it checks for updates, applies anything new, then starts the game automatically.

### What the Updater Does

When launched via the desktop shortcut (with Steam args):
- Checks the latest release on GitHub
- Downloads `tf2vintage-full.zip` for fresh installs or when the install is more than 180 days old
- Downloads `tf2vintage-diff.zip` for incremental updates — only changed files, not the full package
- Applies updates atomically (backs up the live install, swaps in the new files, removes the backup)
- Launches the game automatically after updating (5-second delay so you can read what changed)

When run standalone (double-click without Steam args):
- Runs the same update check and applies any updates
- Pauses at the end so you can read the output before the window closes
- Does not launch the game

### Fallback Behaviour

If GitHub is unreachable (no internet, rate limited, etc.), the updater skips the update check entirely and launches the game normally. Your existing install is never modified if anything goes wrong.

---

## Building from Source

### Prerequisites

- Git with submodule support (the checkout uses `submodules: true`)
- **Windows:** MSVC Build Tools — either full Visual Studio 2022 or the standalone [Build Tools for Visual Studio 2022](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022) with the C++ workload
- **Linux:** GCC multilib and Podman:
  ```bash
  sudo apt install build-essential gcc-multilib g++-multilib podman
  ```
- Go 1.22 or later (for the updater only)

### Clone

```bash
git clone --recurse-submodules https://github.com/TF2V/TF2Vintage.git
cd TF2Vintage
```

> **Note:** The `--recurse-submodules` flag is required. The build will fail without it.

### Compile (Windows)

Dependencies are configured automatically via `ilammy/msvc-dev-cmd`. From a Developer Command Prompt or after running the MSVC environment setup:

```bat
cd src
devtools\bin\vpc.exe /tf2vintage /define:SOURCESDK /define:TF +game /mksln TF2vintage.sln
devenv TF2vintage.sln /Build Release
```

Output lands in `game/tf2vintage/bin/`.

### Compile (Linux)

```bash
cd src
sudo bash createtf2vintage
```

Output lands in `game/tf2vintage/bin/`. ccache is configured automatically during the build to speed up subsequent compiles.

### Build the Updater

```bash
cd tools/updater
go build -o tf2vintage-updater .                     # Linux
GOOS=windows go build -o tf2vintage-updater.exe .   # Windows cross-compile
```


---

## Dedicated Server Setup

### Prerequisites

- Steam with **Source SDK Base 2013 Dedicated Server** installed (free, found in your Steam library Tools section)
- TF2 Vintage base and bin packages extracted to your Sourcemods folder (see [Installation](#installation))

### Launching the Server

**Windows:**
```bat
srcds.exe -game tf2vintage +map ctf_2fort +maxplayers 24 -port 27015
```

**Linux:**
```bash
./srcds_run -game tf2vintage +map ctf_2fort +maxplayers 24 -port 27015
```

> **Note:** `srcds_run` automatically restarts the server on crash. Use `./srcds_linux` directly if you prefer to manage restarts yourself.

> **Tip:** Add `+exec server_extra.cfg` to your launch parameters instead of relying on `server.cfg` alone — this preserves community vote settings between map changes.

### Common Launch Parameters

| Parameter | Description |
|---|---|
| `-game tf2vintage` | Required — points srcds at the TF2 Vintage mod |
| `+map <mapname>` | Starting map (e.g. `ctf_2fort`, `cp_granary`) |
| `-port 27015` | UDP port to listen on (default 27015) |
| `+maxplayers 24` | Player slot count |
| `-tickrate 66` | Server tickrate (66 recommended, 128 for powerful servers) |
| `+sv_pure 0` | Allows custom content on clients |
| `-nohltv` | Disables SourceTV if not needed |
| `+exec server_extra.cfg` | Load TF2 Vintage settings and preserve vote state between maps |

### Basic server.cfg

Located at `tf2vintage/cfg/server.cfg`. Create it if it does not exist:

```
hostname "My TF2 Vintage Server"
sv_password ""
rcon_password "changeme"

sv_cheats 0
sv_lan 0

log on
sv_logbans 1
sv_logecho 1
sv_logfile 1
```

Round timing, team balance, damage, and crits are all handled in `server_extra.cfg` — no need to duplicate them here.

### TF2 Vintage Settings (server_extra.cfg)

`tf2vintage/cfg/server_extra.cfg` is pre-generated by the game and contains all TF2 Vintage-specific configuration. Every cvar is commented inline. Key areas it covers:

- **Win conditions** — timelimit, round limits, CTF caps
- **Team balancing** — autoteambalance, unbalance limits
- **Damage & crits** — random spread, crit chances, melee crits, miss chance
- **Item & weapon settings** — year restrictions, reskins, cut weapons, Demoknight items, cosmetics
- **Legacy era mechanics** — airblast behaviour, backstab logic, building upgrades and hauling, Uber rate, grenade contact detonation, and many other per-era toggles

### Era Presets

One of TF2 Vintage's most powerful server features is its preset system. Rather than tweaking individual cvars, you can load a complete era or gameplay profile by uncommenting a single line in `server_extra.cfg`.

**Quality-of-life era presets** (gameplay tuned to a specific year, full weapon roster still available):
```
exec server/qol2007.txt   // 2007 — no airblast, hauling, or upgrades
exec server/qol2008.txt   // 2008 — airblast enabled
exec server/qol2009.txt   // 2009 — airblast + upgrades + capcrits
exec server/qol2010.txt   // 2010 onward — all features enabled
// ...up to qol2017.txt
```

**Full conversion presets** (locks weapons, gameplay, and mapcycle to an era):
```
exec server/2007_conversion.txt    // Pre-item TF2 purist experience
exec server/premann_conversion.txt // Pre-Mannconomy with loadouts
exec server/f2p_conversion.txt     // Modern gameplay with custom maplist
// ...and more
```

**Combat presets:**
```
exec server/combatstandard.txt  // Casual crits, damage spread
exec server/combatcomp.txt      // Competitive — no random crits or spread
exec server/esportmode.txt      // Strict league mode
```

**Class restriction presets:**
```
exec server/class6s.txt          // 6v6 — one medic and demoman per team
exec server/classhighlander.txt  // Highlander — one of each class per team
exec server/classultiduo.txt     // Ultiduo — soldier + medic only
// ...and many more
```

### Map Rotation

`server_extra.cfg` also controls the mapcycle. Uncomment one line to set your rotation:

```
// By game mode
mapcyclefile maps/mapcycle_ctf.txt
mapcyclefile maps/mapcycle_cp.txt
mapcyclefile maps/mapcycle_koth.txt

// By era
mapcyclefile maps/mapcycle_stock.txt     // Launch TF2 maps only
mapcyclefile maps/mapcycle_premann.txt   // Pre-Mannconomy maps
mapcyclefile maps/mapcycle_f2p.txt       // All TF2V compatible maps

// Custom rotation (recommended)
mapcyclefile maps/mapcycle_custom.txt
```

Only one `mapcyclefile` line should be active at a time — later entries override earlier ones.


---

## Crash Reports

When TF2 Vintage crashes, files are automatically saved to:

```
tf2vintage/logs/crashes/
  crash_YYYYMMDD_HHMMSS.txt        ← human-readable report (all platforms)
  crash_YYYYMMDD_HHMMSS.dmp        ← minidump (Windows only)
  crash_YYYYMMDD_HHMMSS.map        ← address list for addr2line (Linux only)
```

The crash handler registers before the game engine loads, so it catches startup crashes — including the "loads libraries then immediately exits" case that the engine's own handler misses.

> **Players:** You do not need to download anything extra. Crash files are saved automatically. If you encounter a crash, open an issue and paste the `.txt` file.

> **Debug symbols are opt-in.** Symbol downloads are disabled by default. To enable them, run `tf2vintage-updater.exe --enable-symbols` (Windows) or `./tf2vintage-updater --enable-symbols` (Linux) from `bin/x64/`. Symbols are stored in `bin/x64/symbols/` and updated automatically alongside the game on each release.

### For players — reporting a crash

Open an issue at https://github.com/TF2V/TF2Vintage/issues/new and paste the full contents of the `.txt` file. Include what you were doing when it happened and whether it is reproducible.

The `.txt` file alone — without any extra tools — contains enough information to identify most crashes:

- The signal or exception type and address
- A stack trace showing module name and offset for each frame
- A full list of loaded modules at crash time
- The exact TF2 Vintage build version

### For contributors — reading a crash with symbols

Symbols give you function names, source file names, and line numbers on top of the module+offset the `.txt` already provides. They are published for **every release** (weekly and full) as `tf2vintage-symbols.zip`.

**Windows (.dmp + symbols):**

1. Find the release matching the crash — build date and commit are in the `.txt` under `TF2Vintage:`
2. Download `tf2vintage-symbols.zip` from that release and extract it
3. Open the `.dmp` in WinDbg: run `.sympath+ <path to extracted symbols\windows>` then `!analyze -v`
4. Or in Visual Studio: open the `.dmp`, set symbol path in Debug → Options → Debugging → Symbols, click "Run Native Only"

**Linux (.map + symbols):**

The `.map` file contains one `libpath 0xoffset` entry per stack frame. Two options:

Option 1 — using `.so` files already on your system (no download, approximate):
```bash
while IFS=' ' read -r lib offset; do
  echo "$lib $offset:"
  addr2line -e "$lib" -f -C "$offset"
done < crash_YYYYMMDD_HHMMSS.map
```

Option 2 — using published `.debug` files (exact source lines):
```bash
# Download tf2vintage-symbols.zip from the matching release, then:
addr2line -e symbols/linux/server.so.debug -f -C 0x<offset>
```

### For server operators — Linux symbol quality

The crash report notes whether the stack walk used `libunwind` or the fallback `backtrace()`. Installs with `libunwind` produce more reliable stack traces through signal frames:

```bash
# Debian/Ubuntu
apt install libunwind-dev

# RHEL/Fedora
dnf install libunwind
```

This is optional — the crash handler works without it, but the stack trace will be shallower on some crash types.

---

## Contributing

Pull requests are welcome. For larger changes, open an issue first to discuss what you'd like to change.

- Keep code style consistent with the surrounding code
- Test on both Windows and Linux where possible
- CI will build and validate your changes automatically on push

### Debug Symbols

Symbol downloads are off by default. Contributors working on crash reports can enable them:

```
tf2vintage-updater.exe --enable-symbols   # Windows
./tf2vintage-updater --enable-symbols     # Linux
```

Symbols are downloaded alongside the next update and stored in `bin/<platform>/symbols/`. Disable with `--disable-symbols`.

### Nightly Builds

Nightly builds contain the latest debug binaries without waiting for a full release cycle. To opt in:

```
tf2vintage-updater.exe --enable-nightly   # Windows
./tf2vintage-updater --enable-nightly     # Linux
```

The updater checks for a nightly build silently on each launch and applies it if newer than the installed build. Game assets are never changed by a nightly — only the compiled binaries. Disable with `--disable-nightly`.

---

## License

Team Fortress 2 Vintage  
Copyright (C) 2017-2026 Team Fortress 2 Vintage Team  
https://github.com/TF2V/TF2Vintage

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

---

## Source 1 SDK License

Source SDK Copyright(c) Valve Corp.

THIS DOCUMENT DESCRIBES A CONTRACT BETWEEN YOU AND VALVE CORPORATION ("Valve"). PLEASE READ IT BEFORE DOWNLOADING OR USING THE SOURCE ENGINE SDK ("SDK"). BY DOWNLOADING AND/OR USING THE SOURCE ENGINE SDK YOU ACCEPT THIS LICENSE. IF YOU DO NOT AGREE TO THE TERMS OF THIS LICENSE PLEASE DON'T DOWNLOAD OR USE THE SDK.

You may, free of charge, download and use the SDK to develop a modified Valve game running on the Source engine. You may distribute your modified Valve game in source and object code form, but only for free. Terms of use for Valve games are found in the Steam Subscriber Agreement located here: http://store.steampowered.com/subscriber_agreement/

You may copy, modify, and distribute the SDK and any modifications you make to the SDK in source and object code form, but only for free. Any distribution of this SDK must include this LICENSE file and thirdpartylegalnotices.txt.

Any distribution of the SDK or a substantial portion of the SDK must include the above copyright notice and the following:

DISCLAIMER OF WARRANTIES. THE SOURCE SDK AND ANY OTHER MATERIAL DOWNLOADED BY LICENSEE IS PROVIDED "AS IS". VALVE AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES WITH RESPECT TO THE SDK, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, TITLE AND FITNESS FOR A PARTICULAR PURPOSE.

LIMITATION OF LIABILITY. IN NO EVENT SHALL VALVE OR ITS SUPPLIERS BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS) ARISING OUT OF THE USE OF OR INABILITY TO USE THE ENGINE AND/OR THE SDK, EVEN IF VALVE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

If you would like to use the SDK for a commercial purpose, please contact Valve at sourceengine@valvesoftware.com.
