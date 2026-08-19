# MURMUR — FAQ

---

## Installation

**Logic doesn't show MURMUR after install.**

Quit Logic completely, reopen, then **Settings → Plug-in Manager → Reset & Rescan Selection**. Confirm `~/Library/Audio/Plug-Ins/Components/MURMUR.component` exists.

**macOS says the installer can't be opened.**

Right-click the `.pkg` → **Open** → confirm **Open**. Or remove quarantine: `xattr -dr com.apple.quarantine ~/Downloads/MURMUR-*.pkg`

**Presets folder is empty in BROWSE.**

Re-run the installer or check:
`~/Library/Application Support/MURMUR/Presets/factory/` — should contain 900 `.pw8` files (including `Interstellar/`).

---

## Playing

**No sound when I play keys.**

- Confirm the track is record-armed or input monitoring is on.
- Check Logic's channel strip level and MURMUR master gain.
- Load a factory preset (init may be silent until configured).

**Mod wheel does nothing.**

Every factory preset supports mod wheel → filter. If dead: confirm MIDI channel matches (MP11SE zone channel 1 = Logic track channel 1). Check PLAY badge for "Mod Wheel (CC1)".

**Pads stack and don't stop on legato chords.**

Ben MVP 1.0.0 fixes pad polyphony and amp legato. Reinstall the latest `.pkg` and resync presets.

**Notes hang when I stop Logic transport.**

Update to Ben MVP 1.0.0 — transport stop triggers all-sound-off.

---

## MP11SE / MIDI

**Knobs on my Kawai don't affect MURMUR.**

MP11SE knobs don't send MIDI until programmed in a **MIDI OUT zone**. See [`../KAWAI_MP11SE.md`](../KAWAI_MP11SE.md).

**Expression pedal moves Macro 2 and filter — is that a bug?**

Intentional. CC11 drives both the mod-matrix Expression source and Macro 2 for Smart Control feedback.

---

## Presets & saving

**Where are my saved patches?**

Wherever you chose in **SAVE** — typically your project folder or a user presets directory.

**Do favorites survive reinstall?**

Yes — `~/Library/Application Support/MURMUR/favorites.json` is not removed by reinstall unless you delete Application Support manually.

**Can I use presets in another DAW?**

`.pw8` files are MURMUR-native. Other DAWs need MURMUR installed (AU or VST3).

---

## Technical

**Intel Mac?**

Ben MVP ships **arm64 only**. Intel builds are not CI'd yet.

**VST3 in Logic?**

Logic on Mac uses AU. Download a `--full` maintainer build for VST3 (Ableton, etc.).

**How many parameters can I automate?**

800+ APVTS parameters including macros, filter, operators, FX. Mod routes are patch data (saved in `.pw8`), not individual automation IDs.

**Is MURMUR the same as MURMUR?**

MURMUR is the **product name** for the shipping instrument. The engine and repo use the internal name MURMUR (`pw8::`).

---

## Support & updates

**How do I update?**

[GitHub Releases](https://github.com/cbackstrom80/murmur-app/releases) → download latest `.pkg` → run → restart DAW.

**Where is full documentation?**

- Product guides: `~/Library/Application Support/MURMUR/Docs/product/`
- Logic / MP11SE: `~/Library/Application Support/MURMUR/Docs/`
- Source repo: `docs/product/README.md`

---

## Still stuck?

| Issue | Doc |
|-------|-----|
| Install | [INSTALL.md](../INSTALL.md) |
| Logic setup | [QUICK_START.md](QUICK_START.md) |
| MIDI / controllers | [PERFORMANCE.md](PERFORMANCE.md) |
| Ben MVP checklist | [BEN_MVP.md](../BEN_MVP.md) |
