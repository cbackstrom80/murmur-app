# MURMUR — Requirements & Formats

---

## System requirements (Ben MVP 1.0.0)

| Requirement | Details |
|-------------|---------|
| **macOS** | 13.0 Ventura or later recommended |
| **CPU** | Apple Silicon (M1 / M2 / M3 / M4) — **arm64 build** |
| **DAW** | Logic Pro 10.7+ (primary); any AU host supported |
| **Disk** | ~80 MB for plug-in + presets + wavetables |
| **RAM** | Host-dependent; MURMUR uses up to 32 voice slots per layer |

**Not in Ben MVP release:** Intel (x86_64) Mac builds.

---

## Plug-in formats

| Format | Ben MVP `.pkg` | Maintainer `--full` build |
|--------|----------------|---------------------------|
| **Audio Unit (AU)** | ✅ Included | ✅ |
| **VST3** | — | ✅ |
| **Standalone app** | — | ✅ |

### Logic Pro

Use **AU Instruments → Murmur → MURMUR**.

Logic does not load VST3 on Apple Silicon in the same way as AU — the Ben release targets AU specifically.

### Other DAWs

| DAW | Format | Notes |
|-----|--------|-------|
| Logic Pro | AU | Primary, fully tested |
| GarageBand | AU | Supported |
| Ableton Live | VST3 | Requires `--full` build |
| Reaper | VST3 / AU | Requires appropriate build |

→ Host checklist: [`../HOST_MATRIX.md`](../HOST_MATRIX.md) (maintainer QA)

---

## Install locations

### User install (Ben MVP default — no admin password)

| Item | Path |
|------|------|
| Audio Unit | `~/Library/Audio/Plug-Ins/Components/MURMUR.component` |
| Factory presets | `~/Library/Application Support/MURMUR/Presets/factory/` |
| Showcase presets | `~/Library/Application Support/MURMUR/Presets/showcase/` |
| Wavetables | `~/Library/Application Support/MURMUR/Wavetables/` |
| Documentation | `~/Library/Application Support/MURMUR/Docs/` |
| Favorites | `~/Library/Application Support/MURMUR/favorites.json` |

### System-wide install (maintainer)

| Item | Path |
|------|------|
| Audio Unit | `/Library/Audio/Plug-Ins/Components/MURMUR.component` |
| Content | `/Library/Application Support/MURMUR/` |

Build with: `scripts/build_release_pkg.sh --target system`

---

## Code signing

Ben MVP releases are **ad-hoc signed**. For distribution without Gatekeeper prompts:

- Apple Developer ID Application + Installer certificates
- Notarization via `notarytool`

→ Maintainer flags: `scripts/build_release_pkg.sh --sign "Developer ID Application: …" --notarize`

---

## Updates

1. Download latest `.pkg` from [Releases](https://github.com/cbackstrom80/patchwork-eight/releases).
2. Run installer (overwrites AU + refreshes presets).
3. Restart your DAW.

`version.json` in each release records version and platform metadata.

---

## Uninstall

Remove these folders/files:

```bash
rm -rf ~/Library/Audio/Plug-Ins/Components/MURMUR.component
rm -rf ~/Library/Application\ Support/MURMUR
```

Rescan plug-ins in Logic after removal.

---

## License

See `LICENSE` in the repository root and installer package.

→ [Install guide](../INSTALL.md) · [FAQ](FAQ.md)
