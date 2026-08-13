# Ben MVP — MURMUR 1.0.4

First installable release for **Ben** on **Apple Silicon Mac + Logic Pro + Kawai MP11SE**.

## What’s included

- **MURMUR** Audio Unit (`MURMUR.component`) — Logic-native instrument
- **800 factory presets** (160 each: Basses, Leads, Pads, Sequences, Ambient)
- **Wavetable library** + showcase presets
- **MP11SE performance mapping** out of the box (mod wheel, expression, knobs A–D → macros)
- **Pad playability fix** — legato chords no longer stack infinitely (polyphony + amp legato)
- **Logic Smart Controls** template doc

## Install

1. Download `MURMUR-1.0.4-macOS-arm64.pkg` from [GitHub Releases](https://github.com/cbackstrom80/patchwork-eight/releases).
2. Double-click the `.pkg` → follow the installer (no admin password; installs to your home folder).
3. **Quit Logic** completely, reopen.
4. **Logic → Settings → Plug-in Manager → Reset & Rescan Selection** (or full rescan).
5. New **Software Instrument** track → **AU Instruments → Murmur → MURMUR**.
6. Load presets via **BROWSE** or **LOAD...** in the plug-in PLAY screen.

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
| Factory presets | `~/Library/Application Support/MURMUR/Presets/factory/` |
| Wavetables | `~/Library/Application Support/MURMUR/Wavetables/` |
| Docs | `~/Library/Application Support/MURMUR/Docs/` |

## Updates

Re-run the latest `.pkg` from Releases — it overwrites the AU and refreshes presets. Restart Logic after updating.

## Known limits (MVP)

- **Apple Silicon (arm64) only** in this release build
- **Audio Unit only** (no VST3/Standalone in Ben pkg)
- **Ad-hoc signed** — if Gatekeeper blocks: right-click `.pkg` → Open
- **No in-plugin auto-update** yet — check GitHub Releases manually
