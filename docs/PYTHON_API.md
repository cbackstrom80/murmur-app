# Python API

**PARTIAL, IMPLEMENTED and smoke-tested** against the desired API shape from the
master spec. Built via pybind11, off by default
(`-DPW8_BUILD_PYTHON_BINDINGS=ON`, requires Python development headers).

```python
import sys; sys.path.insert(0, "build/python")  # see BUILD.md for the real install story
import patchwork_eight as pw8

patch = pw8.Patch.load("content/presets/dark-bass.pw8")
patch.layer_a.operator(0).engine = "wavetable"
patch.layer_a.operator(0).classic_waveform = "square"   # only meaningful while engine == "classic"
patch.name = "My Dark Bass Variant"

# Stateless one-shot render:
result = pw8.render(patch, "content/test_midi/bass-line.mid", sample_rate=48000, bpm=105, quality="offline")
print(result["metrics"]["peak"], len(result["audio"]))   # audio: interleaved [L, R, L, R, ...] floats

# Or via a live Engine instance (reuses the loaded patch across calls):
engine = pw8.Engine(sample_rate=48000)
engine.load_patch(patch)
result2 = engine.render("content/test_midi/bass-line.mid", bpm=105)

# Live note streaming:
engine.note_on(60, channel=0, velocity=100)
block = engine.process(512)   # interleaved floats, pulls from the engine's current live state
engine.note_off(60)
```

## Coverage

| Feature | Status |
|---|---|
| `Patch.load` / `Patch.from_json` / `Patch.to_json` / `Patch.save` | IMPLEMENTED |
| `Patch.name` / `Patch.category` / `Patch.seed` properties | IMPLEMENTED |
| `patch.layer_a` / `patch.layer_b` -> `Layer` (gain/pan/attack/decay/sustain/release) | IMPLEMENTED |
| `layer.operator(i)` -> `Operator` (engine/classic_waveform/classic_morph/frequency_ratio/fixed_frequency_hz/key_track/level) | IMPLEMENTED |
| `Engine(sample_rate)`, `load_patch`, `note_on`/`note_off`/`all_notes_off` | IMPLEMENTED |
| `Engine.process(num_frames)` -- pull live audio | IMPLEMENTED |
| `Engine.render(midi=..., bpm=..., quality=...)` -- reuses the loaded engine | IMPLEMENTED |
| `pw8.render(patch, midi, ...)` -- stateless one-shot render | IMPLEMENTED |
| Programmatic algorithm graph editing from Python | PLANNED |
| numpy zero-copy buffer protocol for `audio`/`process()` (currently returns a plain Python `list[float]`, which is simple but copies) | PLANNED |
| Proper `pip install`-able package layout (`pyproject.toml`, wheel building, `__init__.py`) -- currently a bare `.so` you add to `sys.path` | PLANNED |
| Macro/mod-matrix editing from Python | PLANNED (waits on the mod matrix itself, see MODULATION.md) |

## Module-level constants

`patchwork_eight.__engine_version__` (str), `patchwork_eight.__patch_schema_version__` (int).

## Build output layout

The extension is built directly as `build/python/patchwork_eight.cpython-<ver>-<platform>.so`
(not nested inside a same-named subdirectory) so `sys.path.insert(0, "build/python"); import patchwork_eight`
resolves straight to the compiled module rather than an accidental Python namespace
package shadowing it -- see `bindings/python/CMakeLists.txt`.
