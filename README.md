[![Build](https://github.com/TF2V/TF2Vintage/actions/workflows/release.yml/badge.svg)](https://github.com/TF2V/TF2Vintage/actions/workflows/release.yml)

# Team Fortress 2 Vintage

A community-driven restoration of classic Team Fortress 2, built on the Source SDK Multiplayer 2013 base.

Development may be slow at times — consider becoming a contributor!

---

## Downloads

TF2 Vintage is distributed as two packages. You need both for a working install.

| Package | Description |
|---|---|
| `tf2vintage-base.zip` | Game assets. Download once; only re-downloads changed files on update. |
| `tf2vintage-bin.zip` | Compiled game code for your platform. Updates frequently with every code change. |

**[→ Latest Release](https://github.com/TF2V/TF2Vintage/releases/latest)**

> **Release schedule:** Full releases publish every 90 days. Weekly updates publish every Thursday at 08:00 UTC if there are new commits — if nothing has changed, no release is published. Dev builds are available as CI artifacts on every commit (90 day expiry) for testing purposes.

---

## Installation

### Prerequisites

- Steam with **Source SDK Base 2013 Multiplayer** installed (free, found in your Steam library Tools section)

### Windows

1. Download `tf2vintage-base.zip` and extract it to your Sourcemods folder. The typical paths are:
   ```
   C:\Program Files (x86)\Steam\steamapps\sourcemods\
   C:\Program Files\Steam\steamapps\sourcemods\
   ```
   If you're unsure which applies, open Steam → **Steam menu** → **Settings** → **Storage** to find your library location. After extraction you should have a `tf2vintage` folder inside `sourcemods\`.

2. Download `tf2vintage-bin.zip` and extract the contents into your `tf2vintage\bin\` folder.

3. Restart Steam. **Team Fortress 2 Vintage** will appear in your library.

### Linux

1. Download `tf2vintage-base.zip` and extract it to your Sourcemods folder:
   ```bash
   unzip tf2vintage-base.zip -d ~/.steam/steam/steamapps/sourcemods/
   ```

2. Download `tf2vintage-bin.zip` and extract it into the same `tf2vintage` folder:
   ```bash
   unzip tf2vintage-bin.zip -d ~/.steam/steam/steamapps/sourcemods/tf2vintage/
   ```

3. Restart Steam. **Team Fortress 2 Vintage** will appear in your library.

---

## Auto-Updater

TF2 Vintage includes an auto-updater (`tf2vintage-updater`) bundled inside the binaries package. It checks for updates on launch and applies only what has changed — base file patches are typically a few KB for config/asset changes, and bin updates replace only the compiled code.

### Setting Up the Steam Launch Option

The updater runs transparently before the game if you set it as a Steam launch option. You only need to do this once.

1. In Steam, right-click **Source SDK Base 2013 Multiplayer** → **Properties**
2. Under **Launch Options**, enter:

**Windows:**
```
"C:\Program Files (x86)\Steam\steamapps\sourcemods\tf2vintage\bin\x64\tf2vintage-updater.exe" %command%
```

**Linux:**
```
~/.steam/steam/steamapps/sourcemods/tf2vintage/bin/x64/tf2vintage-updater %command%
```

> **Note:** If your Steam library is in a non-default location, adjust the path accordingly. The path must point to `tf2vintage-updater` inside your `tf2vintage/bin/x64/` folder. On Windows, wrap the full path in quotes if it contains spaces.

### What the Updater Does

When launched via Steam:
- Checks the latest release on GitHub
- If binaries are out of date, downloads and applies the new bin package for your platform
- If base assets have changed, downloads only a small patch containing the changed files (not the full 963 MB base)
- For installs multiple versions behind, applies patches in sequence to reach the latest version without a full redownload
- Launches the game automatically after updating (3-second delay so you can see what changed)

When run standalone (double-click without Steam):
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
srcds.exe -game tf2vintage +map ctf_2fort +maxplayers 24 -port 27015 -insecure
```

**Linux:**
```bash
./srcds_run -game tf2vintage +map ctf_2fort +maxplayers 24 -port 27015 -insecure
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

## Contributing

Pull requests are welcome. For larger changes, open an issue first to discuss what you'd like to change.

- Keep code style consistent with the surrounding code
- Test on both Windows and Linux where possible
- CI will build and validate your changes automatically on push

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
