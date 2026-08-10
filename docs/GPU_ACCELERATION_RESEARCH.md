# GPU Acceleration Research (CUDA / GPU Audio)

Researched at the user's request. This is a **decision record, not an implementation
plan** -- Patchwork Eight remains CPU-only in this pass, deliberately, for reasons
explained below. Sources:
[attackmagazine.com/features/long-read/gpu-audio-is-powering-plugins-with-your-graphics-card](https://www.attackmagazine.com/features/long-read/gpu-audio-is-powering-plugins-with-your-graphics-card-is-it-time-to-finally-say-goodbye-to-latency/),
[musicradar.com/news/gpu-audio-vst-plugins](https://www.musicradar.com/news/gpu-audio-vst-plugins),
[musictech.com/news/gear/gpu-audio-fir-convolution-gpu-full-technology-stack](https://musictech.com/news/gear/gpu-audio-fir-convolution-gpu-full-technology-stack/),
[roadtovr.com/nvidias-vrworks-audio-brings-physically-based-3d-gpu-accelerated-sound](https://www.roadtovr.com/nvidias-vrworks-audio-brings-physically-based-3d-gpu-accelerated-sound/),
[gpuopen.com/archived/true-audio-next](https://gpuopen.com/archived/true-audio-next/).

## What GPU Audio (the company) actually does

GPU Audio (Los Angeles) ships an SDK that offloads DSP work from CPU to GPU (NVIDIA
first, AMD support since announced, macOS support in progress) while holding
sub-1ms added latency -- demonstrated with 64+ simultaneous instances of a
GPU-accelerated FIR convolution reverb VST3 plugin with no glitches. The hard
problem they solve is architectural, not just "more cores": GPUs are natively
**SIMD, batch-oriented** (thousands of cores executing the *same* instruction on
different data, tuned for throughput), while realtime audio is closer to
**MIMD, latency-oriented** (many small, different, tightly-deadlined tasks). Their
core IP is a custom **"Scheduler"** that re-batches classical DSP algorithms onto
GPU cores while still meeting per-block realtime deadlines -- described as the thing
that "kept [GPU audio] out of viability for the last 20 years" before they solved
it. Their own stated best-fit workloads: convolution reverb, dynamic spatial
reverbs, room simulation, and neural-network inference (e.g. amp modeling) --
i.e. **workloads that are themselves already massively, uniformly parallel**, not
single-voice recursive DSP.

NVIDIA VRWorks Audio and AMD TrueAudio Next are the older, adjacent lineage: both
use GPU ray-tracing hardware/APIs (OptiX / Radeon Rays) to simulate real-time sound
*propagation* through a 3D scene (reflection, occlusion, material absorption) for
game/VR audio -- physically-based acoustic rendering, not synthesis, but directly
relevant to Patchwork Eight's PLANNED Resonator/Spectral engine (physical modeling)
and reverb (Phase 10/11).

## Relevance to Patchwork Eight's architecture

The subsystems that would benefit most from this style of acceleration if pursued
are exactly the ones already flagged in this repo as needing a "vectorized... not
independent oscillator objects" design, or that are inherently large-parallel-N
workloads:

- **Additive** (Engine Type 4, PLANNED, target 64-128 partials -- Zebra 3's 1024-partial
  engine noted in `docs/COMPETITIVE_ANALYSIS.md` is the kind of scale where GPU
  parallelism across partials starts to matter).
- **Granular** (Engine Type 6, PLANNED) -- many independent, short-lived, uniformly-processed
  grains is a textbook GPU-friendly shape.
- **Resonator/Spectral** (Engine Type 8, PLANNED) -- modal/comb resonator banks are
  many parallel identical filter instances, and any future physical-modeling
  exciter/propagation work is exactly VRWorks/TrueAudio Next's use case.
- **Reverb** (Phase 11, PLANNED) -- FDN/convolution reverbs are GPU Audio's flagship
  demonstrated workload.
- **Polyphony itself**: this engine's existing architecture already renders every
  voice's 8-node algorithm graph independently and sums them
  (`render::Engine::process()`) -- structurally a data-parallel operation across
  voices *today*, just executed serially on CPU. That shape would map onto a GPU
  scheduler without an architectural rewrite, if one were ever added underneath.

What would **not** obviously benefit: the Classic oscillator, DAHDSR envelope,
Filter 1, and the algorithm graph's per-sample recursive state (feedback edges,
filter integrator state) -- these are small, tightly sequential, per-sample-dependent
computations where GPU dispatch overhead would likely dominate any parallel gain,
which matches GPU Audio's own emphasis on convolution/reverb/inference rather than
single-voice subtractive synthesis DSP.

## Why this repository stays CPU-only for now

1. **Portability.** `docs/ARCHITECTURE.md` already commits to CLAP, Linux, embedded/ARM,
   and render-farm-worker targets. CUDA is NVIDIA-only; a hard CUDA dependency in
   `pw8_core` would break Apple Silicon (no CUDA), most embedded targets, and any
   Linux render-farm node without an NVIDIA GPU -- directly contradicting that
   commitment. Any GPU path would have to be a strictly optional *additional*
   backend behind the existing CPU implementation, never a replacement for it.
2. **The hard part is scheduling, not math.** GPU Audio's differentiator is
   specifically the realtime-deadline-aware GPU scheduler bridging the SIMD/MIMD
   mismatch -- that is real, nontrivial systems engineering (their own framing: two
   decades of the industry considering it nonviable), not something to casually
   bolt on. Attempting a naive "just dispatch DSP kernels to CUDA" implementation
   without solving that scheduling problem would likely make latency *worse*, not
   better.
3. **No forcing performance need yet.** `docs/TESTING.md`'s benchmark numbers show a
   32-voice full-patch render at 96 kHz taking ~16ms (Debug build, unoptimized) to
   render 1 second of audio -- a 60x+ realtime margin already, on CPU alone, before
   any of the PLANNED heavier engines (additive/granular/resonator) exist to create
   pressure on that budget.

## If pursued later

The natural shape, consistent with the rest of this codebase's "quality modes"
architecture (`render::QualityMode`, already specified): an optional GPU backend
selected at prepare-time (not per-sample), used only for the specific
massively-parallel workloads identified above (additive partial synthesis,
granular grain processing, resonator banks, convolution reverb), with the existing
CPU path remaining the default, portable, always-correct implementation. This is
noted here for future reference; no roadmap phase has been added or changed as a
result of this research.
