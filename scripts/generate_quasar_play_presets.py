#!/usr/bin/env python3
"""Generate 20 distinct QUASAR PLAY factory presets (stereo-split + macro KOINS).

Run from repo root:
    python3 scripts/generate_quasar_play_presets.py
"""
from __future__ import annotations

import json
import pathlib

REPO = pathlib.Path(__file__).resolve().parent.parent
OUT_DIR = REPO / "content" / "presets" / "quasar" / "play"

DEFAULTS: dict[str, float | int] = {
    "mix": 0.78,
    "qsr1Level": 0.68,
    "qsr2Level": 0.58,
    "cntrLevel": 0.82,
    "inputSplitHpfHz": 120.0,
    "cntrHpfHz": 80.0,
    "qsr1Height": 0.0,
    "qsr1Angle": 55.0,
    "qsr1Distance": 0.38,
    "qsr2Height": 0.0,
    "qsr2Angle": 305.0,
    "qsr2Distance": 0.42,
    "qsr1RoomAmount": 0.38,
    "qsr1RoomSize": 1.05,
    "qsr1RoomDamping": 0.52,
    "qsr2RoomAmount": 0.34,
    "qsr2RoomSize": 1.12,
    "qsr2RoomDamping": 0.48,
    "quasarDelayTimeMs": 420.0,
    "quasarDelayFeedback": 0.34,
    "quasarDelayVolume": 0.2,
    "quasarOutputMode": 0,
    "quasarCrossfeed": 0.0,
    "quasarDelaySync": 0,
    "quasarDelaySyncDivision": 2,
    "orbitMacro": 0.5,
    "spreadMacro": 0.5,
    "sidechainToQsr2": 1,
}


def p(**overrides: float | int) -> dict[str, float | int]:
    params = dict(DEFAULTS)
    params.update(overrides)
    return params


PRESETS: list[dict] = [
    {
        "file": "001-wide-stereo-split.quasar",
        "name": "WIDE STEREO SPLIT",
        "description": "Classic L→QSR1 / R→QSR2 headphone orbit. Neutral macros — start here.",
        "tags": ["play", "wide", "headphone", "default"],
        "params": p(qsr1Angle=72.0, qsr2Angle=288.0, qsr1Distance=0.45, qsr2Distance=0.48, spreadMacro=0.62),
    },
    {
        "file": "002-tight-center-anchor.quasar",
        "name": "TIGHT CENTER ANCHOR",
        "description": "Strong CNTR anchor, narrow spread macro, feeds stay close for focused mixes.",
        "tags": ["play", "narrow", "cntr", "mix-ready"],
        "params": p(cntrLevel=0.95, qsr1Level=0.52, qsr2Level=0.48, spreadMacro=0.18, qsr1Distance=0.22, qsr2Distance=0.24),
    },
    {
        "file": "003-left-intimate.quasar",
        "name": "LEFT INTIMATE",
        "description": "L feed close and low; R feed distant — asymmetric stereo split portrait.",
        "tags": ["play", "asymmetric", "left"],
        "params": p(qsr1Angle=88.0, qsr1Distance=0.18, qsr1Height=-0.12, qsr2Angle=250.0, qsr2Distance=0.72, qsr2Height=0.08),
    },
    {
        "file": "004-right-intimate.quasar",
        "name": "RIGHT INTIMATE",
        "description": "Mirror of LEFT INTIMATE — R channel whisper, L channel in the distance.",
        "tags": ["play", "asymmetric", "right"],
        "params": p(qsr1Angle=110.0, qsr1Distance=0.68, qsr1Height=0.1, qsr2Angle=272.0, qsr2Distance=0.16, qsr2Height=-0.1),
    },
    {
        "file": "005-overhead-halo.quasar",
        "name": "OVERHEAD HALO",
        "description": "Both feeds lifted above the head — celestial halo for pads and vocals.",
        "tags": ["play", "height", "pad", "vocal"],
        "params": p(qsr1Height=0.72, qsr2Height=0.68, qsr1Angle=40.0, qsr2Angle=320.0, qsr1RoomAmount=0.52, qsr2RoomAmount=0.48),
    },
    {
        "file": "006-floor-pressure.quasar",
        "name": "FLOOR PRESSURE",
        "description": "Low elevation, heavier CNTR HPF split — sub stays centered, mids orbit low.",
        "tags": ["play", "low", "bass-safe"],
        "params": p(qsr1Height=-0.55, qsr2Height=-0.48, cntrHpfHz=95.0, inputSplitHpfHz=160.0, qsr1Distance=0.32, qsr2Distance=0.35),
    },
    {
        "file": "007-cathedral-wash.quasar",
        "name": "CATHEDRAL WASH",
        "description": "Large room both paths, long delay tail — Interstellar cathedral character.",
        "tags": ["play", "room", "cathedral", "delay"],
        "params": p(
            qsr1RoomAmount=0.62,
            qsr2RoomAmount=0.58,
            qsr1RoomSize=2.4,
            qsr2RoomSize=2.6,
            qsr1RoomDamping=0.38,
            qsr2RoomDamping=0.35,
            quasarDelayTimeMs=680.0,
            quasarDelayFeedback=0.42,
            quasarDelayVolume=0.28,
        ),
    },
    {
        "file": "008-closet-dry.quasar",
        "name": "CLOSET DRY",
        "description": "Tiny room, short distance — dry binaural placement without wash.",
        "tags": ["play", "dry", "intimate"],
        "params": p(
            qsr1RoomAmount=0.12,
            qsr2RoomAmount=0.1,
            qsr1RoomSize=0.35,
            qsr2RoomSize=0.38,
            qsr1Distance=0.14,
            qsr2Distance=0.16,
            quasarDelayVolume=0.05,
            qsr1RoomDamping=0.72,
            qsr2RoomDamping=0.68,
        ),
    },
    {
        "file": "009-orbit-ready.quasar",
        "name": "ORBIT READY",
        "description": "Orbit macro pre-biased — sweep ORBIT knob for continuous head-spin motion.",
        "tags": ["play", "orbit", "macro", "motion"],
        "params": p(orbitMacro=0.72, qsr1Angle=120.0, qsr2Angle=240.0, spreadMacro=0.55),
    },
    {
        "file": "010-spread-maximum.quasar",
        "name": "SPREAD MAXIMUM",
        "description": "Spread macro wide open — exaggerated L/R separation for synths and drum buses.",
        "tags": ["play", "spread", "macro", "wide"],
        "params": p(spreadMacro=0.88, qsr1Angle=95.0, qsr2Angle=265.0, qsr1Distance=0.55, qsr2Distance=0.58),
    },
    {
        "file": "011-narrow-beam.quasar",
        "name": "NARROW BEAM",
        "description": "Collapsed spread macro — both feeds converge toward front-center phantom image.",
        "tags": ["play", "narrow", "mono-ish"],
        "params": p(spreadMacro=0.12, qsr1Angle=12.0, qsr2Angle=348.0, qsr1Distance=0.28, qsr2Distance=0.3),
    },
    {
        "file": "012-behind-head.quasar",
        "name": "BEHIND HEAD",
        "description": "Both feeds behind the listener — uncanny rear orbit for ambient and FX returns.",
        "tags": ["play", "rear", "ambient"],
        "params": p(qsr1Angle=145.0, qsr2Angle=215.0, qsr1Distance=0.52, qsr2Distance=0.5, qsr1Height=0.05, qsr2Height=-0.05),
    },
    {
        "file": "013-front-stage.quasar",
        "name": "FRONT STAGE",
        "description": "Both feeds in front — club PA feel in headphones without speaker crossfeed.",
        "tags": ["play", "front", "stage"],
        "params": p(qsr1Angle=335.0, qsr2Angle=25.0, qsr1Distance=0.4, qsr2Distance=0.38, qsr1Height=-0.08, qsr2Height=-0.06),
    },
    {
        "file": "014-diagonal-cross.quasar",
        "name": "DIAGONAL CROSS",
        "description": "Extreme diagonal: L front-left elevated, R rear-right lowered — X-shaped headphone field.",
        "tags": ["play", "cross", "experimental"],
        "params": p(qsr1Angle=55.0, qsr1Height=0.45, qsr1Distance=0.35, qsr2Angle=235.0, qsr2Height=-0.42, qsr2Distance=0.62),
    },
    {
        "file": "015-tempo-echo-orbit.quasar",
        "name": "TEMPO ECHO ORBIT",
        "description": "Tempo-synced 1/4 delay with moderate orbit — rhythmic spatial pump for electronic material.",
        "tags": ["play", "delay", "sync", "electronic"],
        "params": p(quasarDelaySync=1, quasarDelaySyncDivision=2, quasarDelayFeedback=0.38, quasarDelayVolume=0.32, orbitMacro=0.58),
    },
    {
        "file": "016-canyon-tail.quasar",
        "name": "CANYON TAIL",
        "description": "Long delay, high feedback freeze-adjacent tail — canyon reflections in binaural space.",
        "tags": ["play", "delay", "ambient", "long"],
        "params": p(quasarDelayTimeMs=1450.0, quasarDelayFeedback=0.78, quasarDelayVolume=0.35, qsr1RoomAmount=0.48, qsr2RoomAmount=0.44),
    },
    {
        "file": "017-dry-panner.quasar",
        "name": "DRY PANNER",
        "description": "Minimal room and delay — pure ITD/ILD binaural placement for mixing transparency.",
        "tags": ["play", "dry", "utility", "mix"],
        "params": p(
            mix=0.65,
            qsr1RoomAmount=0.0,
            qsr2RoomAmount=0.0,
            quasarDelayVolume=0.0,
            qsr1Level=0.75,
            qsr2Level=0.75,
            cntrLevel=0.7,
        ),
    },
    {
        "file": "018-speaker-glue.quasar",
        "name": "SPEAKER GLUE",
        "description": "Speaker output mode with crossfeed — safer on loudspeakers, still spatial on cans.",
        "tags": ["play", "speaker", "crossfeed"],
        "params": p(quasarOutputMode=1, quasarCrossfeed=0.35, spreadMacro=0.42, qsr1Angle=48.0, qsr2Angle=312.0),
    },
    {
        "file": "019-sidechain-qsr2.quasar",
        "name": "SIDECHAIN QSR2",
        "description": "QSR2 aux routing enabled — route kick/bass sidechain in Logic; main L stays QSR1.",
        "tags": ["play", "sidechain", "aux", "logic"],
        "params": p(sidechainToQsr2=1, qsr2Level=0.82, qsr1Level=0.55, qsr2Angle=270.0, qsr2Distance=0.28, cntrLevel=0.88),
    },
    {
        "file": "020-neon-club.quasar",
        "name": "NEON CLUB",
        "description": "Bright HPF, tight room, wide spread — synthwave club stereo with punchy center.",
        "tags": ["play", "club", "synth", "bright"],
        "params": p(
            inputSplitHpfHz=220.0,
            cntrHpfHz=110.0,
            spreadMacro=0.78,
            qsr1RoomSize=0.65,
            qsr2RoomSize=0.7,
            qsr1RoomAmount=0.22,
            qsr2RoomAmount=0.2,
            quasarDelayTimeMs=280.0,
            quasarDelayFeedback=0.28,
            quasarDelayVolume=0.18,
            qsr1Angle=80.0,
            qsr2Angle=280.0,
        ),
    },
]


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for spec in PRESETS:
        doc = {
            "schemaVersion": 1,
            "metadata": {
                "name": spec["name"],
                "author": "MURMUR Sound Design",
                "description": spec["description"],
                "tags": spec["tags"],
            },
            "params": spec["params"],
        }
        path = OUT_DIR / spec["file"]
        path.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")
        print(f"  wrote {path.relative_to(REPO)}")
    print(f"\nGenerated {len(PRESETS)} QUASAR PLAY presets in {OUT_DIR.relative_to(REPO)}/")


if __name__ == "__main__":
    main()
