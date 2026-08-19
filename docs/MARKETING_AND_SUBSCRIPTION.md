# Marketing & subscription strategy

Patchwork Audio sells **one subscription** through two acquisition lanes: third-party
synth packs (top of funnel) and **MURMUR** (moat + retention).

## Hierarchy

```
Patchwork Audio          company (legal, billing, support)
├── MURMUR          instrument brand (VST, .pw8 native)
├── Patchforge           marketplace storefront (may rebrand URL later)
└── Patchwork AI         production engine (trust badge, not hero copy)
```

**Primary consumer SKU:** **Patchwork+** ($19/mo) — weekly drops, subscriber catalog.  
**Upgrade SKU:** **Patchwork Pro** ($49/mo) — Plus + MURMUR license + commercial use.

À la carte packs ($14.99) remain as secondary CTAs with copy: *“Plus members got this drop free.”*

## Site IA (Patchforge frontend)

| Route | Purpose |
|-------|---------|
| `/` | Subscription-first home |
| `/plus` | Patchwork+ landing (primary conversion) |
| `/starfighter` | VST product page + demo download |
| `/patches` | Unified catalog (VITAL · MURMUR · future formats) |
| `/pack/:slug` | Pack detail + Plus upsell |
| `/library` | Entitlements |
| `/cart` | Checkout (Plus pre-selected from `/plus` traffic) |
| `/catalog` | Redirect → `/patches` |

## Funnel

1. **Free preview** — no account; waveform + quality score builds trust.
2. **Free pack / demo synth** — email capture.
3. **Patchwork+ trial** — weekly drops, back catalog while active.
4. **Pro upgrade** — MURMUR license bundled.

Every page: one primary button → **Start Patchwork+**.

## Marketing lanes

### Third-party synth packs (VITAL, Serum, …)

- Genre event drops (“Dark Techno Bass Week”)
- Honest preview content (rendered WAV, not DAW screenshots)
- Free sampler + email list
- Creator collab packs
- SEO: DAW + genre long-tail landing pages

### MURMUR (native .pw8)

- **Verifiable Sound Objects** — patch + receipt + preview from same render pipeline
- Demo plugin → Pro upsell in-product
- Plus-exclusive MURMUR drops
- Live patch design streams using PLAY mode UI
- Founding-member Pro pricing at launch

## Ingest & catalog

- `scripts/patchforge_ingest.py` — MURMUR `.pw8` → catalog + WAV
- Future: format adapters per target synth (VITAL, Serum)
- Catalog field: `synth: "STARFIGHTER" | "VITAL" | …`
- Future field: `access: "public" | "plus" | "pro"` for gated drops

## Implementation phases

| Phase | Deliverable |
|-------|-------------|
| 1 | Strategy doc + `/plus` + `/starfighter` + nav/home rewrite |
| 2 | Stripe subscriptions + entitlements API |
| 3 | Plus-gated catalog + in-plugin demo → account funnel |
| 4 | Multi-synth ingest adapters |
| 5 | Annual plan, referral, win-back campaigns |

## Copy anchors

**Company:** Patchwork Audio  
**Subscription:** Patchwork+  
**Instrument:** MURMUR  
**Tagline:** *Scored sound for every synth — and one synth built for the forge.*
