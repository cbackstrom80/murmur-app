"""Plain-Python .pw8 patch construction/editing -- the actual business logic
behind the MCP tools in server.py, kept separate and undecorated so it's
directly unit-testable without an MCP transport in the loop (see
smoke_test.py). No juce/pw8_core dependency: this builds the same JSON shape
PatchSerializer.cpp reads, matching the structure every hand-authored patch
in content/presets/ already uses.
"""
from __future__ import annotations

import json
import time
from pathlib import Path

from patch_schema import (
    BASE_OPERATOR_FIELDS, ENGINE_EXTRA_FIELDS, ENGINE_NAMES,
    EFFECT_FIELDS, EFFECT_TYPE_IDS, EFFECT_TYPE_NAMES,
    FILTER_MODE_IDS, FILTER_MODE_NAMES,     MASTER_DYNAMICS_FIELDS, MASTER_DYNAMICS_MODE_IDS,
    GENERATIVE_FIELDS, GENERATIVE_STREAM_FIELDS,
    PEAKS_UTILITY_SLOT_FIELDS, PEAKS_UTILITY_MODE_IDS, PEAKS_UTILITY_MODE_NAMES,
    MOD_DEST_IDS, MOD_DEST_NEEDS_TARGET,
    MOD_SCOPE_IDS, MOD_SOURCE_IDS, clamp, resolve_engine, resolve_from_map,
)

REPO_ROOT = Path(__file__).resolve().parent.parent
SCRATCH_DIR = Path(__file__).resolve().parent / "scratch"
SCRATCH_DIR.mkdir(exist_ok=True)


def _blank_operator() -> dict:
    return {
        "engine": 0, "classicWaveform": 2, "classicMorph": -1.0, "pulseWidth": 0.5,
        "wavetableFramePosition": 0.0, "wavetableId": "", "frequencyRatio": 1.0,
        "fixedFrequencyHz": 440.0, "keyTrack": True, "level": 1.0, "pan": 0.0,
        # Every engine's extra fields, all at their defaults -- present on every
        # operator regardless of engine, same flat-struct convention OperatorPatch
        # itself uses (see engine/include/pw8/patch/Patch.hpp).
        "fmModulatorRatio": 1.0, "fmModulatorIndex": 0.5, "fmModulatorFeedback": 0.0,
        "fmModulatorWaveform": 0,
        "noiseVariant": 0, "noiseRate": 200.0,
        "phaseBend": 0.0, "phaseFold": 0.0, "phaseAsymmetry": 0.0, "phaseShape": 0.0,
        "additivePartialCount": 32, "additiveTilt": 0.0, "additiveOddEven": 0.5,
        "additiveStretch": 0.0,
        "resonatorStructure": 0.3, "resonatorDecay": 0.5, "resonatorDamping": 0.5,
        "resonatorBrightness": 0.5, "resonatorModeCount": 6,
        "grainDensity": 20.0, "grainSizeMs": 60.0, "grainPositionJitter": 0.1,
        "grainPitchJitter": 0.0,
    }


def _silent_operator() -> dict:
    op = _blank_operator()
    op["level"] = 0.0
    return op


def _layer_b() -> dict:
    return {
        "operators": [_silent_operator() for _ in range(8)],
        "algorithm": {
            "nodes": [{"id": i, "engine": 0, "isOutput": (i == 0)} for i in range(8)],
            "edges": [],
        },
        "ampEnvelope": {"attackSeconds": 0.002, "decaySeconds": 0.15, "sustainLevel": 0.0, "releaseSeconds": 0.05},
        "gain": 1.0, "pan": 0.0, "width": 1.0, "centerGravity": 0.5,
    }


def default_patch(name: str, description: str = "") -> dict:
    """A fresh, valid, playable patch: 8 silent Classic operators except
    operator 0 (a plain sine at unity level), parallel algorithm (no custom
    routing), a gentle default envelope/filter, no FX, all 8 macros unrouted.
    Every field an MCP tool call doesn't touch keeps this default."""
    ops = [_silent_operator() for _ in range(8)]
    ops[0] = _blank_operator()
    ops[0]["classicWaveform"] = 0  # sine
    return {
        "schemaVersion": 1,
        "metadata": {
            "id": f"pw8-mcp-{int(time.time())}",
            "name": name,
            "author": "MURMUR MCP server",
            "description": description,
            "category": "",
            "moods": [],
            "genres": [],
            "tags": ["mcp-generated"],
            "createdAt": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "engineVersion": "0.1.0",
            "schemaVersion": 1,
            "seed": int(time.time()) % 1_000_000,
            "lineage": [],
        },
        "layerMode": 0,
        "layerMorph": 0.0,
        "layerA": {
            "operators": ops,
            "algorithm": {
                "nodes": [{"id": i, "engine": 0, "isOutput": True} for i in range(8)],
                "edges": [],
            },
            "ampEnvelope": {"attackSeconds": 0.01, "decaySeconds": 0.2, "sustainLevel": 0.7,
                             "releaseSeconds": 0.3, "curveShape": 2.0},
            "unison": {"mode": 0, "voices": 1},
            "filter1": {"enabled": False, "mode": 0, "cutoffHz": 4000.0, "resonance": 0.1, "keyTrack": 0.0},
            "lfo1": {"waveform": 0, "mode": 0, "rateHz": 1.0, "syncDivisionIndex": 4, "phaseOffset": 0.0},
            "lfo2": {"waveform": 0, "mode": 0, "rateHz": 1.0, "syncDivisionIndex": 4, "phaseOffset": 0.0},
            "modRoutes": [],
            "gain": 1.0, "pan": 0.0, "width": 1.0, "centerGravity": 0.5,
            "insertEffects": [],
        },
        "layerB": _layer_b(),
        "voiceSettings": {"polyphony": 8, "masterGain": 1.0, "a4Hz": 440.0},
        "locks": {},
        "macros": [{"id": f"m{i+1}", "name": f"Macro {i+1}", "description": "Reserved -- not yet routed.", "value": 0.0}
                    for i in range(8)],
        "arpeggiator": {"enabled": False},
        "masterEffects": [],
        "masterDynamics": {"enabled": False},
    }


def scratch_path(patch_id: str) -> Path:
    """Resolves patch_id to a file under SCRATCH_DIR, guarding against path
    traversal since patch_id can come from an LLM-controlled tool call."""
    if "/" in patch_id or "\\" in patch_id or ".." in patch_id:
        raise ValueError(f"invalid patch_id '{patch_id}' -- must be a bare filename with no path separators")
    # New scratch patches always use the current .murmur extension. Legacy .pw8
    # patch_ids passed in explicitly still resolve correctly (no forced rename of
    # an already-suffixed id) -- see docs/REBRAND_MURMUR.md.
    if not patch_id.endswith(".pw8") and not patch_id.endswith(".murmur"):
        patch_id += ".murmur"
    return SCRATCH_DIR / patch_id


def load_scratch(patch_id: str) -> dict:
    path = scratch_path(patch_id)
    if not path.exists():
        raise FileNotFoundError(f"no scratch patch '{patch_id}' -- call create_patch first")
    return json.loads(path.read_text())


def write_scratch(patch_id: str, patch: dict) -> Path:
    path = scratch_path(patch_id)
    path.write_text(json.dumps(patch, indent=2))
    return path


def create_patch(name: str, description: str = "") -> tuple[str, dict]:
    patch = default_patch(name, description)
    slug = "".join(c if c.isalnum() else "-" for c in name.lower()).strip("-") or "patch"
    patch_id = f"{slug}-{int(time.time() * 1000) % 100000}.murmur"
    write_scratch(patch_id, patch)
    return patch_id, patch


def set_operator(patch_id: str, index: int, engine, params: dict | None) -> list[str]:
    if not 0 <= index <= 7:
        raise ValueError("index must be 0-7 (8 operators per layer)")
    patch = load_scratch(patch_id)
    engine_id = resolve_engine(engine)
    engine_name = ENGINE_NAMES[engine_id]
    op = patch["layerA"]["operators"][index]
    op["engine"] = engine_id

    warnings: list[str] = []
    field_specs = dict(BASE_OPERATOR_FIELDS)
    field_specs.update(ENGINE_EXTRA_FIELDS.get(engine_name, {}))
    for key, value in (params or {}).items():
        if key not in field_specs:
            warnings.append(f"ignored unknown field '{key}' for engine {engine_name}")
            continue
        clamped, warn = clamp(value, field_specs[key])
        if warn:
            warnings.append(f"{key}: {warn}")
        op[key] = clamped

    write_scratch(patch_id, patch)
    return warnings


def add_mod_route(patch_id: str, source, destination, target_index: int = 0,
                   amount: float = 1.0, scope="voice") -> list[str]:
    patch = load_scratch(patch_id)
    warnings: list[str] = []
    source_id = resolve_from_map(source, MOD_SOURCE_IDS, "mod source")
    dest_id = resolve_from_map(destination, MOD_DEST_IDS, "mod destination")
    scope_id = resolve_from_map(scope, MOD_SCOPE_IDS, "mod scope")
    if dest_id in MOD_DEST_NEEDS_TARGET and not 0 <= target_index <= 7:
        raise ValueError(f"destination requires target_index 0-7, got {target_index}")
    route = {"source": source_id, "destination": dest_id, "targetIndex": target_index,
             "amount": amount, "scope": scope_id}
    patch["layerA"]["modRoutes"].append(route)
    write_scratch(patch_id, patch)
    return warnings


def set_effect(patch_id: str, layer: str, slot: int, effect_type, params: dict | None) -> list[str]:
    patch = load_scratch(patch_id)
    warnings: list[str] = []
    type_id = resolve_from_map(effect_type, EFFECT_TYPE_IDS, "effect type")
    type_name = EFFECT_TYPE_NAMES[type_id]

    if layer == "insert":
        key, max_slots = "insertEffects", 3
    elif layer == "master":
        key, max_slots = "masterEffects", 4
        patch.setdefault("masterEffects", [])
    else:
        raise ValueError("layer must be 'insert' or 'master'")
    if not 0 <= slot < max_slots:
        raise ValueError(f"{layer} slot must be 0-{max_slots - 1}")

    slots = patch["layerA"][key] if key == "insertEffects" else patch[key]
    while len(slots) <= slot:
        slots.append({"type": 0, "mix": 1.0})

    entry = {"type": type_id, "mix": 1.0}
    field_specs = EFFECT_FIELDS.get(type_name, {})
    for key2, value in (params or {}).items():
        if key2 == "mix":
            clamped, warn = clamp(value, {"range": [0.0, 1.0]})
            if warn:
                warnings.append(f"mix: {warn}")
            entry["mix"] = clamped
            continue
        if key2 not in field_specs:
            warnings.append(f"ignored unknown field '{key2}' for effect {type_name}")
            continue
        clamped, warn = clamp(value, field_specs[key2])
        if warn:
            warnings.append(f"{key2}: {warn}")
        entry[key2] = clamped

    slots[slot] = entry
    write_scratch(patch_id, patch)
    return warnings


def set_envelope(patch_id: str, attack: float, decay: float, sustain: float,
                  release: float, curve: float = 2.0) -> list[str]:
    patch = load_scratch(patch_id)
    warnings: list[str] = []
    specs = {
        "attackSeconds": {"range": [0.0, 30.0]}, "decaySeconds": {"range": [0.0, 30.0]},
        "sustainLevel": {"range": [0.0, 1.0]}, "releaseSeconds": {"range": [0.0, 30.0]},
        "curveShape": {"range": [0.1, 8.0]},
    }
    values = {"attackSeconds": attack, "decaySeconds": decay, "sustainLevel": sustain,
              "releaseSeconds": release, "curveShape": curve}
    for key, value in values.items():
        clamped, warn = clamp(value, specs[key])
        if warn:
            warnings.append(f"{key}: {warn}")
        patch["layerA"]["ampEnvelope"][key] = clamped
    write_scratch(patch_id, patch)
    return warnings


def set_filter(patch_id: str, enabled: bool = True, mode="lowpass",
                cutoff_hz: float = 2000.0, resonance: float = 0.2, key_track: float = 0.2) -> list[str]:
    patch = load_scratch(patch_id)
    warnings: list[str] = []
    mode_id = resolve_from_map(mode, FILTER_MODE_IDS, "filter mode")
    cutoff_clamped, w1 = clamp(cutoff_hz, {"range": [20.0, 20000.0]})
    res_clamped, w2 = clamp(resonance, {"range": [0.0, 1.0]})
    kt_clamped, w3 = clamp(key_track, {"range": [0.0, 1.0]})
    for w in (w1, w2, w3):
        if w:
            warnings.append(w)
    patch["layerA"]["filter1"] = {
        "enabled": bool(enabled), "mode": mode_id, "cutoffHz": cutoff_clamped,
        "resonance": res_clamped, "keyTrack": kt_clamped,
    }
    write_scratch(patch_id, patch)
    return warnings


def set_macro_koin(patch_id: str, slot: int, name: str, destinations: list[dict],
                   description: str = "") -> list[str]:
    """Configure one feature macro KOIN: macro name/description, uiFocus slot, and mod routes.

    slot: 0–2 (Macro1–3). destinations: list of dicts with keys destination, amount,
    optional target_index (0–7), scope (voice/layer/global). Replaces prior routes for that macro."""
    if not 0 <= slot <= 2:
        raise ValueError("slot must be 0-2 (Macro1-3 feature KOINS)")
    if not destinations:
        raise ValueError("destinations must contain at least one mod route")

    patch = load_scratch(patch_id)
    warnings: list[str] = []
    macro_index = slot
    macro_key = f"macro{macro_index + 1}"
    source_id = MOD_SOURCE_IDS[macro_key]

    patch["macros"][macro_index]["name"] = name[:32]
    if description:
        patch["macros"][macro_index]["description"] = description

    routes = [r for r in patch["layerA"].get("modRoutes", []) if r.get("source") != source_id]
    for dest in destinations:
        dest_name = dest.get("destination")
        if dest_name is None:
            raise ValueError("each destination requires 'destination'")
        dest_id = resolve_from_map(dest_name, MOD_DEST_IDS, "mod destination")
        target_index = int(dest.get("target_index", 0))
        amount = float(dest.get("amount", 1.0))
        scope = dest.get("scope", "voice")
        scope_id = resolve_from_map(scope, MOD_SCOPE_IDS, "mod scope")
        if dest_id in MOD_DEST_NEEDS_TARGET and not 0 <= target_index <= 7:
            raise ValueError(f"destination requires target_index 0-7, got {target_index}")
        routes.append({
            "source": source_id,
            "destination": dest_id,
            "targetIndex": target_index,
            "amount": amount,
            "scope": scope_id,
        })
    patch["layerA"]["modRoutes"] = routes

    ui_focus = patch.setdefault("uiFocus", {"maxKnobs": 3, "knobs": []})
    ui_focus["maxKnobs"] = max(int(ui_focus.get("maxKnobs", 3)), slot + 1)
    knobs = [k for k in ui_focus.get("knobs", [])
             if not (k.get("kind") == "macro" and k.get("index") == macro_index)]
    knobs.append({"kind": "macro", "index": macro_index, "label": name[:32]})
    knobs.sort(key=lambda k: k.get("index", 0))
    ui_focus["knobs"] = knobs

    write_scratch(patch_id, patch)
    return warnings


def set_spread_bundle(patch_id: str, slot: int, name: str, destinations: list[dict],
                      description: str = "") -> list[str]:
    """Alias for set_macro_koin with PoliMATHS Spread semantics — one macro fans to many destinations."""
    return set_macro_koin(patch_id, slot, name, destinations, description)


def _normalize_macro_values(raw) -> list[float]:
    """Accept length-8 list or partial list; pad with zeros."""
    if raw is None:
        return []
    if not isinstance(raw, (list, tuple)):
        raise ValueError("macroValues must be a list of 0-8 floats in 0..1")
    out = [float(clamp(v, {"range": [0.0, 1.0]})[0]) for v in raw[:8]]
    return out


def set_master_dynamics(patch_id: str, params: dict | None) -> list[str]:
    """Configure Streams-style master bus dynamics (top-level masterDynamics)."""
    patch = load_scratch(patch_id)
    warnings: list[str] = []
    md = dict(patch.get("masterDynamics", {"enabled": False}))

    if params:
        for key, value in params.items():
            if key == "mode":
                mode_key = str(value).strip().lower().replace(" ", "_")
                if mode_key not in MASTER_DYNAMICS_MODE_IDS:
                    raise ValueError(
                        f"unknown masterDynamics mode '{value}' — valid: {sorted(MASTER_DYNAMICS_MODE_IDS)}"
                    )
                md["mode"] = mode_key
                continue
            if key not in MASTER_DYNAMICS_FIELDS:
                warnings.append(f"unknown masterDynamics field '{key}' ignored")
                continue
            spec = MASTER_DYNAMICS_FIELDS[key]
            if spec.get("type") == "bool":
                md[key] = bool(value)
            elif spec.get("type") == "string":
                md[key] = str(value)
            else:
                clamped, warn = clamp(float(value), spec)
                md[key] = clamped
                if warn:
                    warnings.append(warn)

    patch["masterDynamics"] = md
    write_scratch(patch_id, patch)
    return warnings


def set_generative(patch_id: str, params: dict | None = None,
                   streams: list[dict] | None = None) -> list[str]:
    """Configure Marbles-style generative mod sources (top-level generative on patch)."""
    patch = load_scratch(patch_id)
    warnings: list[str] = []
    gen = dict(patch.get("generative", {}))

    for key, value in (params or {}).items():
        if key not in GENERATIVE_FIELDS:
            warnings.append(f"unknown generative field '{key}' ignored")
            continue
        spec = GENERATIVE_FIELDS[key]
        if spec.get("type") == "bool":
            gen[key] = bool(value)
        else:
            clamped, warn = clamp(value, spec)
            if warn:
                warnings.append(f"{key}: {warn}")
            gen[key] = clamped

    if streams is not None:
        merged: list[dict] = []
        for i, stream in enumerate(streams[:4]):
            entry: dict = {}
            for key, value in stream.items():
                if key not in GENERATIVE_STREAM_FIELDS:
                    warnings.append(f"stream[{i}] unknown field '{key}' ignored")
                    continue
                clamped, warn = clamp(value, GENERATIVE_STREAM_FIELDS[key])
                if warn:
                    warnings.append(f"streams[{i}].{key}: {warn}")
                entry[key] = clamped
            merged.append(entry)
        if merged:
            gen["streams"] = merged

    patch["generative"] = gen
    write_scratch(patch_id, patch)
    return warnings


def set_utility_peaks(patch_id: str, slot: int, params: dict | None = None) -> list[str]:
    """Configure a Peaks utility slot (0–1) on top-level utilityPeaks."""
    if slot not in (0, 1):
        raise ValueError("slot must be 0 or 1")
    patch = load_scratch(patch_id)
    warnings: list[str] = []
    peaks = dict(patch.get("utilityPeaks", {}))
    slots: list[dict] = list(peaks.get("slots", [{}, {}]))
    while len(slots) < 2:
        slots.append({})
    entry = dict(slots[slot])

    for key, value in (params or {}).items():
        if key == "mode":
            mode_id = resolve_from_map(value, PEAKS_UTILITY_MODE_IDS, "peaks utility mode")
            entry["mode"] = mode_id
            continue
        if key not in PEAKS_UTILITY_SLOT_FIELDS:
            warnings.append(f"unknown utilityPeaks field '{key}' ignored")
            continue
        spec = PEAKS_UTILITY_SLOT_FIELDS[key]
        if spec.get("type") == "bool":
            entry[key] = bool(value)
        else:
            clamped, warn = clamp(value, spec)
            if warn:
                warnings.append(f"{key}: {warn}")
            entry[key] = clamped

    slots[slot] = entry
    peaks["slots"] = slots
    patch["utilityPeaks"] = peaks
    write_scratch(patch_id, patch)
    return warnings


def set_morph_koin(patch_id: str, label: str, keyframes: list[dict],
                   default_position: float = 0.0, description: str = "",
                   curve: str = "linear", wrap: bool = False,
                   position: float | None = None,
                   autoplay_source: str = "none",
                   macro_koins: list[dict] | None = None) -> list[str]:
    """Configure a Frames-style morph KOIN: top-level morphKoin metadata + uiFocus morph entry.

    keyframes: 2–4 dicts with name (required), optional position (0..1), macroValues (list),
    paramOverrides (dict of path -> float). Does not run morph DSP (Horizon 3).

    macro_koins: optional list of {slot, name, destinations, description?} passed to set_macro_koin
    after morph metadata is written (typical: 2 Spread-style macros alongside morph)."""
    if not 2 <= len(keyframes) <= 4:
        raise ValueError("keyframes must contain 2-4 snapshot states")
    for i, kf in enumerate(keyframes):
        if not kf.get("name"):
            raise ValueError(f"keyframes[{i}] requires 'name'")

    patch = load_scratch(patch_id)
    warnings: list[str] = []
    default_clamped, w0 = clamp(default_position, {"range": [0.0, 1.0]})
    if w0:
        warnings.append(w0)
    if position is None:
        pos = default_clamped
    else:
        pos, w1 = clamp(position, {"range": [0.0, 1.0]})
        if w1:
            warnings.append(w1)

    allowed_curves = {"linear", "smooth", "step"}
    curve_norm = curve if curve in allowed_curves else "linear"
    if curve not in allowed_curves:
        warnings.append(f"curve '{curve}' unknown — using linear")

    parsed_keyframes: list[dict] = []
    for kf in keyframes:
        entry: dict = {"name": str(kf["name"])[:32]}
        if "position" in kf:
            pos_kf, w = clamp(float(kf["position"]), {"range": [0.0, 1.0]})
            if w:
                warnings.append(w)
            entry["position"] = pos_kf
        mv = _normalize_macro_values(kf.get("macroValues"))
        if mv:
            while len(mv) < 8:
                mv.append(0.0)
            entry["macroValues"] = mv
        overrides = kf.get("paramOverrides")
        if overrides:
            if not isinstance(overrides, dict):
                raise ValueError("paramOverrides must be a dict of path -> float or override object")
            parsed_overrides: dict = {}
            for k, v in overrides.items():
                if isinstance(v, (int, float)):
                    parsed_overrides[str(k)] = float(v)
                elif isinstance(v, dict):
                    parsed_overrides[str(k)] = {
                        "value": float(v.get("value", 0.0)),
                        "easing": str(v.get("easing", "")),
                        "response": str(v.get("response", "")),
                    }
                else:
                    raise ValueError("paramOverrides values must be float or {value,easing?,response?}")
            entry["paramOverrides"] = parsed_overrides
        parsed_keyframes.append(entry)

    autoplay = str(autoplay_source or "none")
    patch["morphKoin"] = {
        "label": label[:32],
        "description": description,
        "defaultPosition": default_clamped,
        "position": pos,
        "curve": curve_norm,
        "wrap": bool(wrap),
        "autoplaySource": autoplay,
        "keyframes": parsed_keyframes,
    }

    ui_focus = patch.setdefault("uiFocus", {"maxKnobs": 3, "knobs": []})
    ui_focus["maxKnobs"] = 3
    knobs = [k for k in ui_focus.get("knobs", []) if k.get("kind") != "morph"]
    knobs.insert(0, {"kind": "morph", "label": label[:32]})
    ui_focus["knobs"] = knobs[:3]

    write_scratch(patch_id, patch)

    if macro_koins:
        for mk in macro_koins:
            slot = int(mk["slot"])
            macro_warnings = set_macro_koin(
                patch_id,
                slot=slot,
                name=mk["name"],
                destinations=mk["destinations"],
                description=mk.get("description", ""),
            )
            warnings.extend(macro_warnings)

    return warnings


def add_morph_keyframe(patch_id: str, name: str, position: float,
                       macro_values: list[float] | None = None,
                       param_overrides: dict | None = None) -> list[str]:
    """Append one morph keyframe (DESIGN cap 16). Preserves existing keyframes."""
    patch = load_scratch(patch_id)
    morph = patch.setdefault("morphKoin", {"label": "MORPH", "keyframes": []})
    keyframes = morph.setdefault("keyframes", [])
    if len(keyframes) >= 16:
        raise ValueError("morph keyframe cap is 16")

    pos, w0 = clamp(position, {"range": [0.0, 1.0]})
    warnings: list[str] = []
    if w0:
        warnings.append(w0)

    entry: dict = {"name": str(name)[:32], "position": pos}
    mv = _normalize_macro_values(macro_values)
    if mv:
        while len(mv) < 8:
            mv.append(0.0)
        entry["macroValues"] = mv

    if param_overrides:
        parsed: dict = {}
        for k, v in param_overrides.items():
            if isinstance(v, (int, float)):
                parsed[str(k)] = float(v)
            elif isinstance(v, dict):
                parsed[str(k)] = {
                    "value": float(v.get("value", 0.0)),
                    "easing": str(v.get("easing", "")),
                    "response": str(v.get("response", "")),
                }
            else:
                raise ValueError("param_overrides values must be float or override object")
        entry["paramOverrides"] = parsed

    keyframes.append(entry)
    keyframes.sort(key=lambda k: k.get("position", 0.0))
    write_scratch(patch_id, patch)
    return warnings


def remove_morph_keyframe(patch_id: str, index: int) -> list[str]:
    """Remove morph keyframe by index (0-based). Requires at least one keyframe remains for PLAY."""
    patch = load_scratch(patch_id)
    morph = patch.get("morphKoin")
    if not morph or not morph.get("keyframes"):
        raise ValueError("patch has no morphKoin keyframes")

    keyframes = morph["keyframes"]
    if index < 0 or index >= len(keyframes):
        raise ValueError(f"keyframe index {index} out of range (0..{len(keyframes) - 1})")

    keyframes.pop(index)
    write_scratch(patch_id, patch)
    return []


def explain_patch(patch: dict) -> str:
    """A human/LLM-readable summary -- the same information MCP callers would
    otherwise have to reconstruct by reading raw JSON field-by-field."""
    lines = []
    meta = patch.get("metadata", {})
    lines.append(f"{meta.get('name', '(unnamed)')} -- {meta.get('description', '')}".rstrip(" -"))
    ops = patch["layerA"]["operators"]
    for i, op in enumerate(ops):
        if op.get("level", 0.0) <= 0.0:
            continue
        engine_name = ENGINE_NAMES.get(op.get("engine", 0), "?")
        lines.append(f"  Op {i}: {engine_name}, level={op.get('level'):.2f}, ratio={op.get('frequencyRatio'):.3f}")
    env = patch["layerA"]["ampEnvelope"]
    lines.append(f"  Envelope: A={env['attackSeconds']:.3f}s D={env['decaySeconds']:.3f}s "
                 f"S={env['sustainLevel']:.2f} R={env['releaseSeconds']:.3f}s")
    f1 = patch["layerA"]["filter1"]
    if f1.get("enabled"):
        lines.append(f"  Filter1: {FILTER_MODE_NAMES.get(f1['mode'], '?')} @ {f1['cutoffHz']:.0f}Hz, "
                     f"res={f1['resonance']:.2f}")
    routes = patch["layerA"].get("modRoutes", [])
    for r in routes:
        src = next((k for k, v in MOD_SOURCE_IDS.items() if v == r["source"]), r["source"])
        dst = next((k for k, v in MOD_DEST_IDS.items() if v == r["destination"]), r["destination"])
        lines.append(f"  Mod: {src} -> {dst} (amount={r['amount']})")
    morph = patch.get("morphKoin")
    if morph and morph.get("keyframes"):
        names = " ↔ ".join(k.get("name", "?") for k in morph["keyframes"])
        pos = morph.get("position", morph.get("defaultPosition", 0.0))
        lines.append(f"  Morph KOIN: {morph.get('label', 'MORPH')} [{names}] @ {pos:.2f}")
    for slot in patch["layerA"].get("insertEffects", []):
        name = EFFECT_TYPE_NAMES.get(slot.get("type", 0), "?")
        if name != "bypass":
            lines.append(f"  Insert FX: {name} (mix={slot.get('mix', 1.0):.2f})")
    for slot in patch.get("masterEffects", []):
        name = EFFECT_TYPE_NAMES.get(slot.get("type", 0), "?")
        if name != "bypass":
            lines.append(f"  Master FX: {name} (mix={slot.get('mix', 1.0):.2f})")
    md = patch.get("masterDynamics")
    if md and md.get("enabled"):
        mode = md.get("mode", "envelope")
        lines.append(
            f"  Master dynamics: {mode} (mix={md.get('mix', 1.0):.2f}, "
            f"threshold={md.get('thresholdDb', -12.0):.1f}dB)"
        )
    gen = patch.get("generative")
    if gen:
        lines.append(
            f"  Generative: T={gen.get('clockTRateHz', 0.47):.2f}Hz "
            f"X={gen.get('clockXRateHz', 4.7):.2f}Hz "
            f"dejaVu={gen.get('dejaVu', True)} corr={gen.get('correlation', 0.0):.2f}"
        )
    peaks = patch.get("utilityPeaks")
    if peaks and peaks.get("slots"):
        for i, slot in enumerate(peaks["slots"]):
            if slot.get("enabled"):
                mode = PEAKS_UTILITY_MODE_NAMES.get(slot.get("mode", 0), "?")
                lines.append(f"  Utility peaks slot {i}: {mode}")
    return "\n".join(lines)
