# Email to Ben — MURMUR 1.4.3

**Subject:** MURMUR 1.4.3 ready — Logic installer (DMG)

---

Hi Ben,

MURMUR **1.4.3** is ready. This is the full Cadillac build — plug-in, all factory content, standalone app, and a proper drag-and-readme installer.

**Download:** attach `MURMUR-1.4.3-macOS-arm64-full.dmg` from GitHub Releases (or send directly).

## Install (5 minutes)

1. Open the **DMG**
2. Read **`1 — READ ME FIRST.txt`** (optional but helpful)
3. Double-click **`Install MURMUR.pkg`** and follow the prompts  
   - No admin password — installs to your home folder
   - If macOS blocks it: **right-click the pkg → Open → Open**
4. **Quit Logic completely** (Cmd+Q), then reopen
5. **Logic → Settings → Plug-in Manager → Reset & Rescan Selection**
6. New track → **Software Instrument → AU Instruments → Murmur → MURMUR**

## Using presets

- Click **BROWSE** in the top bar, or
- Click the **preset name** in the header, or
- Press **⌘B**

That opens the Preset Explorer (search, categories, mood/genre filters).

## Presets to show off

**Interstellar (175)** — filter category **INTERSTELLAR** or search `cinematic`:
- **CATHEDRAL NEBULA** — hold C3–C5, sweep mod wheel
- **CORNFIELD CHASE** — lead lines, great with scope view
- **VOID CATHEDRAL** (`Interstellar/Spatial/`) — headphones spatial pad

**Hoover Bass (28)** — filter category **BASS**, genre **hoover-bass**, or search `hoover`:
- **CLUB HOOVER** — the classic rave hoover (Basses/087)
- **RAVE DRIVE**, **ACID SURGE** — more aggressive hoover variants

Full demo list: `~/Library/Application Support/MURMUR/Docs/BEN_DEMO_PRESETS.md`

## What's new

- iPad-style PLAY screen (scope + master volume deck + 6 macros)
- Preset Explorer with metadata filtering
- 1800+ factory presets + wavetables + design FX all bundled
- Standalone app included (drag MURMUR.app to Applications to test without Logic)

## If Logic still shows an old version

1. Quit Logic fully
2. Plug-in Manager → Reset & Rescan Selection (hold Option for full reset)
3. Re-run the installer pkg

Docs install automatically to:

```
~/Library/Application Support/MURMUR/Docs/
```

Let me know how it goes in Logic — especially preset browsing and the PLAY layout.

— Chris
