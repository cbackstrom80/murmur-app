# Ben Request — Master Motion Lab (Figma → Code)

**From:** Ben (1.4.4 soak, Aug 2026)  
**Ask:** Dedicated screen for **Master Envelope** + **4 Master LFOs**  
**Design owner:** Chris / Figma  
**Engine status:** DSP ready — 8 env + 8 LFO per layer; LAYER/GLOBAL scope shared clock

---

## Why this screen

Today master motion is scattered:

| Control | Today | Problem |
|---------|-------|---------|
| Amp envelope (env0) | PLAY Advanced → ENV tab | Only one envelope; labeled "Layer Amp" |
| LFO 1–4 | DESIGN → Dual LFO lab | Hidden from PLAY; Ben lives in Performance |
| LFO 5–8 | Same lab, footer chips | Extra navigation |
| Master bus mod | MOD matrix only | No visual "master motion" story |

Ben wants **one dedicated screen** — clear hierarchy: **MASTER ENVELOPE** first, then **4 MASTER LFOs**.

---

## Figma deliverable (Chris)

**Frame name suggestion:** `murmur-master-motion-lab`  
**Entry:** PLAY Advanced tab **MOTION** or chrome sub-nav **MOT**  
**Size:** 1280×720 modal or full Advanced page (match `DualLfoLabPanel` pattern)

### Layout (top → bottom)

```
┌─────────────────────────────────────────────────────────────┐
│  ← PLAY BOARD          MASTER MOTION LAB           [×]      │
├─────────────────────────────────────────────────────────────┤
│  MASTER ENVELOPE  (hero — must read instantly)              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  Large ADSR visualizer (live, like ObsidianEnvelope) │    │
│  │  D · A · H · D · S · R · CURVE · LEGATO              │    │
│  │  Badge: "MASTER · env0 · drives amp VCA"             │    │
│  └─────────────────────────────────────────────────────┘    │
├─────────────────────────────────────────────────────────────┤
│  MASTER LFOs  (4-up grid)                                   │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │ LFO 1    │ │ LFO 2    │ │ LFO 3    │ │ LFO 4    │       │
│  │ scope    │ │ scope    │ │ scope    │ │ scope    │       │
│  │ WAVE MODE│ │ WAVE MODE│ │ WAVE MODE│ │ WAVE MODE│       │
│  │ RATE/SYNC│ │ RATE/SYNC│ │ RATE/SYNC│ │ RATE/SYNC│       │
│  │ routing  │ │ routing  │ │ routing  │ │ routing  │       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
├─────────────────────────────────────────────────────────────┤
│  [OPEN MOD MATRIX]     Scope: LAYER (shared across voices)  │
└─────────────────────────────────────────────────────────────┘
```

### Master Envelope — clarity requirements

1. **Title:** `MASTER ENVELOPE` — not "Layer Amp" or "Env 0" alone  
2. **Subtitle:** `Shapes overall loudness · all voices`  
3. **Visualizer:** Same language as `AmpEnvelopePanel` / `ObsidianEnvelopeVisualizer` — large, left-weighted  
4. **Knobs:** Figma glow-ring Large (88px) for A/R; Medium for D/H/D/S/Curve  
5. **Legato toggle:** visible, not buried  
6. **Optional:** Mini routing line showing env0 → VCA (icon strip)

### Master LFOs — reuse Dual LFO lab vocabulary

Copy from `murmur-dual-lfo-lab` (`15:247`):

- 180px wireframe scope per column  
- WAVE / MODE / MOTION (rate+depth concentric) knobs  
- SYNC division chip row when TEMPO SYNC  
- Footer routing legend (from mod matrix destinations)

**Difference from DESIGN lab:** Always shows LFO **1–4** (not pair tabs). Footer hint: `LFO 5–8 in DESIGN → Dual LFO`.

### Knobs

Use **Figma glow-ring-knobs** (`21:4`) — decked style only. See `docs/FIGMA_KNOB_MIGRATION.md`.

---

## Code mapping (post-Figma)

| UI | Reuse | New |
|----|-------|-----|
| Page shell | `DualLfoLabPanel` overlay pattern | `MasterMotionLabPanel` |
| Master ENV | `AmpEnvelopePanel` + expand to env0–8 selector later | Hero section |
| LFO columns | `DualLfoLabPanel::LfoColumn` | Extract shared `LfoLabColumn` |
| APVTS | `envelopeParamId(0..7)`, LFO params | Same IDs |
| Scope | `LfoWireframeView` | Same |

---

## Acceptance (Ben)

1. Opens from PLAY without entering DESIGN  
2. **Master Envelope** readable in 2 seconds — title + visual + which env  
3. All 4 LFOs visible without tab switching  
4. Changes audible on held chord + mod wheel  
5. OPEN MOD MATRIX jumps to routes for selected LFO
