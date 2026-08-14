# iPad Port Research — MURMUR

**Status:** Research (Aug 2026)  
**Scope:** Feasibility of porting MURMUR (Patchwork Eight plugin) to iPad — engine, plugin wrapper, UI, AU vs standalone.

---

## Headline

**Feasible as a phased project (effort L)** — JUCE supports iOS AUv3 and standalone; the existing C++ engine and Obsidian UI can largely carry over. **Biggest gaps:** touch-first KOINS/PLAY layout polish, no Logic-style sidechain bus (use mic/Inter-App Audio/input node instead), Quasar CPU on older iPads, and CMake/exporter + App Store pipeline work. **Do not port initially:** MCP server, full Advanced graph editor, VST3, batch preset tooling.

---

## JUCE iOS feasibility

| Area | Assessment |
|------|------------|
| **Engine (`pw8_core`)** | Strong — no JUCE in engine; standard C++20 DSP. Same code path as macOS AU. |
| **Plugin wrapper** | Medium — `PatchworkEightProcessor` is JUCE-based; iOS needs AUv3 + optional standalone target. |
| **UI (`MurmurRootEditor`)** | Medium–Large — Component tree works on iOS; needs layout pass for touch targets, safe areas, no hover-only affordances. |
| **Content / wavetables** | Medium — bundle `content/` in app; replicate `ContentPaths` search roots for iOS bundle. |
| **Sidechain input** | Different UX — AUv3 on iOS exposes **audio input bus** when host supports it (GarageBand, AUM); not Logic bus picker. Mic + IAA/USB audio as modulator source. |
| **Quasar / binaural** | Medium CPU — headphone mode is primary; profile on A14/A17 and throttle quality mode. |

---

## AU vs AUv3 vs standalone iOS

| Format | Recommendation |
|--------|----------------|
| **AUv3 extension** | **Primary** — load in GarageBand, AUM, Cubasis; matches current AU architecture (instrument + sidechain input capability). |
| **Standalone app** | **Phase 2** — same binary core; adds MIDI device picker, built-in keyboard, IAA/AUv3 hosting optional. |
| **Classic AU (.component)** | macOS only — not applicable on iOS. |
| **VST3** | Skip on iOS. |

---

## UI on iPad

| Mode | Port strategy |
|------|----------------|
| **Basic PLAY** | **First ship** — macro KOINS, standard knobs, FX strip; enlarge hit targets (min 44 pt); ring legend stays. |
| **Compact** | Header macro row — already has `setHeaderCompactMode`; verify on 11" and 13" layouts. |
| **Advanced** | **Defer** — mod matrix, graph editor, GLOBAL Quasar deep panel; high information density, mouse-oriented drag paths. |

**Touch KOINS:** decked knobs + mod drag — need long-press for depth popover, larger mod-route hit rings, haptic optional.

---

## Sidechain on iOS (no Logic bus)

- **GarageBand / AUM:** route external track or bus into plugin **audio input** (when host exposes it).
- **Standalone:** `AVAudioEngine` input node or USB audio interface as modulator.
- **Vocoder MVP:** same code path — sidechain pointers from host input buffer; badge copy should say "Audio In" not "Sidechain (AU)" on iOS.
- **Mod matrix `ModSource::Sidechain`:** keep — driven by same follower on input bus.

---

## Engine CPU budget

| Subsystem | iPad note |
|-----------|-----------|
| 8-voice + 8 engines | OK on A14+ in Normal quality |
| Quasar master FX | Profile binaural + room sim; offer **Eco** quality default on iPad |
| Vocoder 8-band | One slot OK; 16 bands + Quasar may need Eco |
| Morph / mod preview | 12 Hz UI timers — fine |

Target: **48 kHz, 256 buffer, <30% CPU** on M1 iPad Pro as stretch goal.

---

## CMake / JUCE exporter changes

1. Add `juce_add_plugin` iOS/AUv3 target or separate `juce_add_console_app` + AUv3 extension (JUCE 7/8 pattern).
2. Enable `JucePlugin_Build_AUv3`, disable VST3/AU v2 on iOS.
3. Code-signing, entitlements, App Groups if sharing presets with standalone.
4. Asset catalog for app icon; bundle `content/wavetables`, factory presets.
5. CI: macOS agent builds `.ipa` or archive; no Windows/Linux for iOS.

---

## Effort estimate

| Phase | Scope | Effort |
|-------|--------|--------|
| **0 — Spike** | AUv3 shell, silence→tone, one preset | **S** (1–2 wk) |
| **1 — PLAY ship** | Basic/Compact UI, factory presets, mic sidechain, Eco quality | **M** (4–6 wk) |
| **2 — FX + vocoder** | Full FX chain, vocoder input routing in hosts | **S–M** |
| **3 — Advanced** | Mod matrix touch, partial GLOBAL | **L** |
| **4 — Polish** | App Store, IAP optional, iCloud presets | **M** |

**Overall to App Store PLAY-quality:** **L** (3–4 months one engineer, part-time QA).

---

## Phased roadmap (recommended)

1. AUv3 + Basic PLAY + Eco quality + bundled Interstellar subset.
2. Sidechain/vocoder via host audio input + follower badge.
3. Standalone with keyboard + AUv3 preset export.
4. Advanced mod/GLOBAL (optional Pro tier).

---

## What NOT to port initially

- MCP server / Cursor automation surface
- Full algorithm graph editor + compile preview
- VST3 build
- Logic-specific copy ("Sidechain (AU)" strings — iOS variants)
- Batch Python preset generators (use pre-baked factory bank)
- Desktop-only install scripts (`install_au_local.sh`)

---

## Risks

| Risk | Mitigation |
|------|------------|
| Host input bus not wired | Document per-host routing; standalone input fallback |
| CPU thermal throttle | Eco quality, reduce polyphony default |
| Touch mod assignment UX | Dedicated "Mod" overlay mode vs drag-from-chip |
| App Store review (AUv3 + IAP) | Clear microphone usage strings if input enabled |
| Quasar headphone assumption | Default Headphone mode; warn on speaker |

---

## References

- [`EXT_OSCILLATOR_AU_THEORY.md`](EXT_OSCILLATOR_AU_THEORY.md) — sidechain bus theory (macOS Logic)
- [`VOCODER_SIDECHAIN_PLAN.md`](VOCODER_SIDECHAIN_PLAN.md) — vocoder + shared sidechain
- JUCE: iOS AUv3 tutorial, `AudioProcessor` bus layout on mobile
