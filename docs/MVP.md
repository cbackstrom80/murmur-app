# MVP — Starfighter / Patchwork Eight

What “MVP” means for this repo: **a shippable vertical slice** you can demo, sell
against (via Patchforge), and regression-gate in one command — not a finished
1.0 product.

Run the gate:

```bash
scripts/mvp_check.sh
```

---

## In scope (MVP)

### Engine (`pw8_core`)

| Capability | Status |
|---|---|
| 8 operator engines (Classic → Resonator) | ✅ |
| 8-node algorithm graph (7 edge types, compile + execute) | ✅ |
| Filter 1 + per-engine filter | ✅ |
| 8 LFOs + 8 envelopes + mod matrix | ✅ |
| Arpeggiator | ✅ |
| 10 FX algorithms (3 insert + 4 master) | ✅ |
| Layer A + **Stack** (Layer B summed) | ✅ |
| Unison (per-layer) | ✅ |
| Deterministic render + seed | ✅ |
| `.pw8` JSON patch format | ✅ |

**Honest limits (clamped, not silent):**

- `layerMode`: only `SingleA` and `Stack` render; other modes clamp to `SingleA`.
- `layerMorph`: stored, not wired to DSP yet.

### Tools

| Tool | Role |
|---|---|
| `pw8-render` | Headless WAV + receipt |
| `pw8-info` / `pw8-graph` | Introspection |
| `pw8-fuzz-render` | Random-patch QA (CI-friendly `--count`) |
| `mcp_server/` | Agent patch build + render (18 tools) |

### Plugin (PLAY mode — OBSIDIAN)

| Surface | Status |
|---|---|
| Patch browser (278 presets, facets, favorites) | ✅ |
| E0–E7 + **GLOBAL** scope | ✅ |
| Tabbed pages: PERF / OSC / FILTER / ENV / MOD / FX | ✅ |
| Engine Sum (GLOBAL → 8 level faders + layer/master gain) | ✅ |
| FX chain flow + swap + wireframes | ✅ |
| Starfighter branding (header ship) | ✅ |
| VST3 + AU (auval / pluginval in CI) | ✅ |
| Standalone | ✅ build; may need `chown` if a root-owned `.app` exists |

### Content

| Asset | Count |
|---|---|
| Factory + showcase presets | 278 |
| Wavetable library | 51 |
| Wavetable refs validated | `scripts/validate_content_refs.py` |

### Commerce (Patchforge — separate repo)

Running storefront at `../patchforge` (Vite/React): catalog, previews, cart,
Patchforge+ pricing. **Real Starfighter catalog** via `scripts/patchforge_ingest.py`
→ `public/catalog.generated.json` + engine-rendered WAV previews (see
`docs/PATCHFORGE_INGEST.md`). Storefront IA: `/plus`, `/starfighter`, `/patches`
(see `docs/MARKETING_AND_SUBSCRIPTION.md` + Patchforge `docs/IA.md`).

---

## Out of scope (post-MVP)

- DESIGN / LAB modes, full graph editor
- Layer morph / dual-layer crossfade DSP
- Filter 2, extra FX wave
- Windows/Linux plugin soak in real DAWs (CI compile-only on Windows)
- Code signing / notarization / App Store
- In-plugin LLM chat box
- Patchforge backend (Stripe, entitlements API) — frontend seam only

---

## Manual checklist (before calling it “demo ready”)

1. **Launch Standalone** or load **VST3/AU** in one DAW you use daily.
2. **GLOBAL → FILTER**: drag Engine Sum faders; hear level changes.
3. **FX tab**: swap insert slots, toggle bypass, pick effect types.
4. **Patch browser**: search `bass`, load preset, prev/next with filter active.
5. **MCP** (optional): `python3 mcp_server/smoke_test.py` — builds + renders laser patch.
6. **Patchforge** (optional): run ingest (`docs/PATCHFORGE_INGEST.md`), then `npm run dev` — audition real Starfighter previews at `:5173`.

---

## One-command verification

`scripts/mvp_check.sh` runs:

1. `cmake --preset dev` + build + **173** unit/regression tests
2. `cmake --preset plugin` + `pw8_plugin` shared library build
3. `python3 scripts/validate_content_refs.py`
4. `python3 mcp_server/smoke_test.py`
5. `pw8-render` smoke on a factory preset
6. `pw8-fuzz-render --count 200` (fast QA sample)

Exit non-zero on any failure.

---

## Known local gotchas

| Issue | Fix |
|---|---|
| Standalone link: `can't write output file` | Old `.app` owned by root: `sudo chown -R "$(whoami)" build/plugin` |
| Wavetables missing in installed plugin | Run `scripts/package_macos.sh` or copy `content/wavetables` to `/Library/Application Support/Patchwork Eight/Wavetables` |
| MCP render fails | Build `dev` preset first so `build/dev/tools/pw8-render` exists |

---

## MVP narrative (one sentence)

**Starfighter** is an eight-engine graph synth with a PLAY cockpit UI, headless
render QA, and an agent toolchain — **Patchforge** is the scored marketplace
layer on top.
