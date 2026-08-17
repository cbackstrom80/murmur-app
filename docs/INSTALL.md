# Installing MURMUR on macOS (Ben)

**MURMUR 1.4.3** — install like any Mac app: download the DMG, double-click the pkg, open Logic. No Xcode or terminal required.

Built for **Apple Silicon (M1/M2/M3/M4)** + **macOS 13+** + **Logic Pro** + **Kawai MP11SE**.

---

## Quick install (recommended — DMG)

1. **Download** from [GitHub Releases](https://github.com/cbackstrom80/patchwork-eight/releases):
   - `MURMUR-1.4.3-macOS-arm64-full.dmg`

2. **Open the DMG** and read `1 — READ ME FIRST.txt`.

3. **Double-click `Install MURMUR.pkg`** and follow the installer.
   - Installs to **your home folder** — **no admin password**
   - Includes AU, VST3, Standalone app, 1800+ factory presets, wavetables, design-FX, and docs

4. **Quit Logic completely**, then reopen.

5. **Rescan plug-ins:** Logic → **Settings → Plug-in Manager → Reset & Rescan Selection**

6. **Add MURMUR:** new Software Instrument track → **AU Instruments → Murmur → MURMUR**

7. **Load a preset:** click **BROWSE**, click the preset name in the header, or press **⌘B**.

Full Ben MVP guide: [`BEN_MVP.md`](BEN_MVP.md)

---

## Logic Pro setup

| Topic | Doc |
|-------|-----|
| **MURMUR product docs** | [`product/README.md`](product/README.md) |
| MP11SE knob + pedal mapping | [`KAWAI_MP11SE.md`](KAWAI_MP11SE.md) |
| Smart Controls 8-knob template | [`LOGIC_SMART_CONTROLS.md`](LOGIC_SMART_CONTROLS.md) |
| All MIDI CC / mod wheel / expression | [`MIDI_CONTROLLERS.md`](MIDI_CONTROLLERS.md) |

Installed copies live at:

```
~/Library/Application Support/MURMUR/Docs/
~/Library/Application Support/MURMUR/Docs/product/
```

---

## What gets installed

| Item | Path |
|------|------|
| Audio Unit | `~/Library/Audio/Plug-Ins/Components/MURMUR.component` |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/MURMUR.vst3` |
| Standalone app | `~/Applications/MURMUR.app` |
| Factory presets (1800+) | `~/Library/Application Support/MURMUR/Presets/factory/` |
| Showcase presets | `~/Library/Application Support/MURMUR/Presets/showcase/` |
| Wavetables | `~/Library/Application Support/MURMUR/Wavetables/` |
| Design FX | `~/Library/Application Support/MURMUR/design-fx/` |
| Docs (Logic, MP11SE, release notes) | `~/Library/Application Support/MURMUR/Docs/` |
| Product documentation | `~/Library/Application Support/MURMUR/Docs/product/` |

The Ben release ships the **full Cadillac installer** (AU + VST3 + Standalone + all content).

---

## Gatekeeper

Release builds are **ad-hoc signed** until Apple Developer ID is configured.

If macOS blocks the installer:

1. **Right-click** the `.pkg` → **Open** → confirm **Open**
2. After install, if the AU fails: **System Settings → Privacy & Security** → allow if prompted
3. Remove quarantine if needed: `xattr -dr com.apple.quarantine ~/Downloads/MURMUR-*.dmg`

---

## Updating

1. Download the latest `.dmg` or `.pkg` from [Releases](https://github.com/cbackstrom80/patchwork-eight/releases)
2. Run the installer (overwrites previous install)
3. **Quit Logic completely**, reopen, **Reset & Rescan Selection**
4. Confirm version **1.4.3** in the plug-in (footer or about)

If Logic still shows an old version after updating, hold **Option** while opening Plug-in Manager for a full reset.

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| "Incompatible" in Logic | Reinstall pkg, reset & rescan plug-ins |
| Crash on open | Reinstall 1.4.3 full pkg; try Standalone app first |
| No presets | Re-run installer; check factory folder has `.pw8` files |
| Can't open preset browser | Click **BROWSE** or press **⌘B** |

See also [`RELEASE_1.4.3.md`](RELEASE_1.4.3.md) for release-specific notes.

---

## Maintainers

One-command Cadillac build:

```bash
scripts/release_gate.sh 1.4.3
```

Verify checklist: [`RELEASE_1.4.3_VERIFY.md`](RELEASE_1.4.3_VERIFY.md)
