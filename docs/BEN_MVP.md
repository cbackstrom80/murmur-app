# Ben MVP — MURMUR 1.4.3

First installable release for **Ben** on **Apple Silicon Mac + Logic Pro + Kawai MP11SE**.

## What's included

- **MURMUR** Audio Unit + VST3 + Standalone app
- **1800+ factory presets** (core banks + 100 Interstellar cinematic)
- **Wavetable library** + design-FX presets + showcase patches
- **Obsidian UI** — PLAY / DESIGN / COMPACT modes
- **Preset Explorer** — search, categories, mood/genre/tag filters, favorites
- **iPad-style PLAY view** — scope + master output deck + 6 performance macros
- **MP11SE performance mapping** (mod wheel, expression, knobs A–D → macros)
- **Logic Smart Controls** template doc

## Install

1. Download `MURMUR-1.4.3-macOS-arm64-full.dmg` from [GitHub Releases](https://github.com/cbackstrom80/patchwork-eight/releases).
2. Open the DMG → double-click **`Install MURMUR.pkg`** (no admin password).
3. **Quit Logic** completely, reopen.
4. **Logic → Settings → Plug-in Manager → Reset & Rescan Selection**.
5. New **Software Instrument** track → **AU Instruments → Murmur → MURMUR**.
6. Open presets: **BROWSE**, click preset name, or **⌘B**.

Copy-paste email for Ben: [`BEN_EMAIL_1.4.3.md`](BEN_EMAIL_1.4.3.md)

## Logic + MP11SE quick start

| Step | Action |
|------|--------|
| Keyboard | Program MP11SE zone per [`KAWAI_MP11SE.md`](KAWAI_MP11SE.md) (knobs → CC74/71/73/91, expression → CC11) |
| Smart Controls | Optional 8-knob layout per [`LOGIC_SMART_CONTROLS.md`](LOGIC_SMART_CONTROLS.md) |
| MIDI | All factory patches: mod wheel, expression, velocity, aftertouch (pads), performance CC map |

Docs installed with the plug-in:

```
~/Library/Application Support/MURMUR/Docs/
~/Library/Application Support/MURMUR/Docs/product/   ← product guides (start here)
```

Product documentation index: [`product/README.md`](product/README.md)

## Installed paths (user install)

| Item | Path |
|------|------|
| Audio Unit | `~/Library/Audio/Plug-Ins/Components/MURMUR.component` |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/MURMUR.vst3` |
| Standalone | `~/Applications/MURMUR.app` |
| Factory presets | `~/Library/Application Support/MURMUR/Presets/factory/` |
| Wavetables | `~/Library/Application Support/MURMUR/Wavetables/` |
| Design FX | `~/Library/Application Support/MURMUR/design-fx/` |
| Docs | `~/Library/Application Support/MURMUR/Docs/` |

## Updates

Re-run the latest `.dmg` or `.pkg` from Releases — it overwrites the AU and refreshes presets. **Quit Logic**, reopen, **Reset & Rescan**.

## Known limits (MVP)

- **Apple Silicon (arm64) only** in this release build
- **Ad-hoc signed** — if Gatekeeper blocks: right-click `.pkg` → Open
- **No in-plugin auto-update** yet — check GitHub Releases manually

## Maintainer build

```bash
scripts/release_gate.sh 1.4.3
```

Verify: [`RELEASE_1.4.3_VERIFY.md`](RELEASE_1.4.3_VERIFY.md)
