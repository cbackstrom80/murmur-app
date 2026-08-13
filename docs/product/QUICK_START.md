# MURMUR — Quick Start

Get from **download** to **playing a preset in Logic Pro** in under five minutes.

---

## 1. Download & install

1. Go to [GitHub Releases](https://github.com/cbackstrom80/patchwork-eight/releases).
2. Download **`MURMUR-1.0.0-macOS-arm64.pkg`** (or the `.dmg` wrapper).
3. **Double-click** the `.pkg` and follow the installer.
   - Installs to your **home folder** — no admin password required.
4. If macOS warns about an unidentified developer: **right-click → Open → Open**.

---

## 2. Open Logic Pro

1. **Quit Logic completely** if it was running during install.
2. Reopen Logic.
3. **Logic → Settings → Plug-in Manager**.
4. Select **MURMUR** (or scan all) → **Reset & Rescan Selection**.

You should see **MURMUR** validated under Audio Units.

---

## 3. Create a MURMUR track

1. **File → New** (or add a track to an existing project).
2. Choose **Software Instrument**.
3. In the instrument slot: **AU Instruments → Murmur → MURMUR**.
4. The MURMUR window opens in **PLAY** mode — dark OBSIDIAN interface with the algorithm graph center stage.

---

## 4. Load a factory preset

**Option A — Browse overlay (recommended)**

1. Click **BROWSE** in the patch bar (top).
2. Type to search (e.g. `pad`, `hoover`, `ambient`).
3. Filter by **category** (Bass, Lead, Pad, Sequence, Ambient).
4. Click a preset to load it.

**Option B — Prev / Next**

1. With BROWSE closed, use **◀ ▶** in the patch bar to step through the current filtered set.

**Option C — Load from disk**

1. Click **LOAD...** to open a file picker.
2. Navigate to factory presets:
   ```
   ~/Library/Application Support/MURMUR/Presets/factory/
   ```

---

## 5. Play

1. **Click the Logic record-arm / input monitor** on the track (or use an external keyboard).
2. Play notes — you should hear the loaded preset immediately.
3. Try the **mod wheel** — every factory patch brightens/darkens the filter by default.
4. Turn the **Knobs of Interest** (six large knobs below the graph) — these are the patch's main performance macros.

---

## 6. Save your session

MURMUR state saves with the **Logic project**. The current patch name appears in the patch bar. Use **SAVE** in the patch bar to write a `.pw8` file to disk.

---

## Keyboard players (Kawai MP11SE)

If you use Ben's MP11SE setup, program the keyboard zone once:

→ [`../KAWAI_MP11SE.md`](../KAWAI_MP11SE.md)

Then mod wheel, expression pedal, and knobs A–D map to MURMUR automatically.

---

## What's next?

| Goal | Read |
|------|------|
| Understand the PLAY screen | [PLAY Mode](PLAY_MODE.md) |
| Explore 800 presets | [Presets](PRESETS.md) |
| Map Logic Smart Controls | [LOGIC_SMART_CONTROLS.md](../LOGIC_SMART_CONTROLS.md) |
| Fix install issues | [FAQ](FAQ.md) |
