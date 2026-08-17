#!/usr/bin/env python3
"""Generate content/design-fx/*.json chip presets for the design FX panel."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "content" / "design-fx"


def write(name: str, payload: dict) -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / name
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


PRESETS = [
    ("sat-01-tube-crunch.json", {"chip": 1, "name": "CLASS A TUBE CRUNCH", "modePill": "TUBE",
     "params": {"SaturationCharacter": 0, "SaturationDrive": 18.0, "Mix": 0.85}}),
    ("sat-02-tape-saturator.json", {"chip": 1, "name": "TAPE SATURATOR", "modePill": "TAPE",
     "params": {"SaturationCharacter": 1, "SaturationDrive": 12.0, "Mix": 0.75}}),
    ("sat-03-diode-crunch.json", {"chip": 1, "name": "DIODE CRUNCH", "modePill": "DIODE",
     "params": {"SaturationCharacter": 2, "SaturationDrive": 24.0, "Mix": 0.9}}),
    ("sat-04-fold-destroyer.json", {"chip": 1, "name": "FOLD DESTROYER", "modePill": "FOLD",
     "params": {"SaturationCharacter": 3, "SaturationDrive": 32.0, "Mix": 0.7}}),
    ("chr-01-wide-ensemble.json", {"chip": 2, "name": "WIDE ENSEMBLE",
     "params": {"ChorusRate": 0.45, "ChorusDepth": 8.0, "Mix": 0.55}}),
    ("chr-02-subtle-doubler.json", {"chip": 2, "name": "SUBTLE DOUBLER",
     "params": {"ChorusRate": 0.2, "ChorusDepth": 3.0, "Mix": 0.4}}),
    ("chr-03-deep-mod.json", {"chip": 2, "name": "DEEP MOD CHORUS",
     "params": {"ChorusRate": 0.8, "ChorusDepth": 12.0, "Mix": 0.65}}),
    ("tape-01-vintage-reel.json", {"chip": 3, "name": "VINTAGE REEL",
     "params": {"TapeDriftRate": 0.25, "TapeDriftDepth": 2.0, "TapeDrive": 4.0, "Mix": 0.5}}),
    ("tape-02-worn-cassette.json", {"chip": 3, "name": "WORN CASSETTE",
     "params": {"TapeDriftRate": 0.6, "TapeDriftDepth": 5.0, "TapeDrive": 8.0, "Mix": 0.6}}),
    ("tape-03-slapback.json", {"chip": 3, "name": "STUDIO SLAPBACK",
     "params": {"TapeDelayMs": 280.0, "TapeFeedback": 0.35, "Mix": 0.45}}),
    ("mood-01-warm-glue.json", {"chip": 4, "name": "WARM ANALOG GLUE", "modePill": "WARM", "params": {"Mix": 0.55}}),
    ("mood-02-dark-resonant.json", {"chip": 4, "name": "DARK RESONANT", "modePill": "DARK", "params": {"Mix": 0.65}}),
    ("mood-03-bright-shimmer.json", {"chip": 4, "name": "BRIGHT SHIMMER", "modePill": "BRIGHT", "params": {"Mix": 0.45}}),
    ("mood-04-acid-resonance.json", {"chip": 4, "name": "ACID RESONANCE", "modePill": "ACID", "params": {"Mix": 0.7}}),
    ("mood-05-ethereal-air.json", {"chip": 4, "name": "ETHEREAL AIR", "modePill": "ETHEREAL", "params": {"Mix": 0.5}}),
    ("fshf-01-bode-cascade.json", {"chip": 5, "name": "BODE CASCADE",
     "params": {"FreqShiftHz": 7.0, "FreqShiftFeedback": 0.55, "Mix": 0.5}}),
    ("fshf-02-sub-octave.json", {"chip": 5, "name": "SUB OCTAVE DRIFT",
     "params": {"FreqShiftHz": -12.0, "FreqShiftFeedback": 0.35, "Mix": 0.55}}),
    ("fshf-03-stereo-phaser.json", {"chip": 5, "name": "STEREO PHASER SHIFT",
     "params": {"FreqShiftHz": 14.0, "FreqShiftFeedback": 0.65, "Mix": 0.48}}),
    ("frac-01-dust-cloud.json", {"chip": 6, "name": "DUST CLOUD", "params": {"FractalMorph": 0.35, "Mix": 0.5}}),
    ("frac-02-crystal-shards.json", {"chip": 6, "name": "CRYSTAL SHARDS", "params": {"FractalMorph": 0.65, "Mix": 0.55}}),
    ("frac-03-grain-storm.json", {"chip": 6, "name": "GRAIN STORM", "params": {"FractalMorph": 0.85, "Mix": 0.6}}),
    ("rev-01-concert-hall.json", {"chip": 7, "name": "LARGE CONCERT HALL", "modePill": "HALL",
     "params": {"ReverbCharacter": 2, "ReverbDecaySeconds": 3.5, "ReverbSize": 1.4, "Mix": 0.45}}),
    ("rev-02-plate-vocal.json", {"chip": 7, "name": "PLATE VOCAL", "modePill": "PLATE",
     "params": {"ReverbCharacter": 1, "ReverbDecaySeconds": 2.2, "ReverbDiffusion": 0.85, "Mix": 0.4}}),
    ("rev-03-small-room.json", {"chip": 7, "name": "SMALL ROOM", "modePill": "ROOM",
     "params": {"ReverbCharacter": 3, "ReverbDecaySeconds": 1.1, "ReverbPreDelayMs": 8.0, "Mix": 0.35}}),
    ("rev-04-spring-tank.json", {"chip": 7, "name": "SPRING TANK", "modePill": "SPRING",
     "params": {"ReverbCharacter": 4, "ReverbModDepth": 0.45, "Mix": 0.42}}),
    ("rev-05-shimmer-pad.json", {"chip": 7, "name": "SHIMMER PAD", "modePill": "SHIMMER",
     "params": {"ReverbCharacter": 0, "ReverbModDepth": 0.85, "ReverbHighRatio": 0.95, "Mix": 0.5}}),
    ("eq-01-master-cleanup.json", {"chip": 8, "name": "MASTER CLEAN-UP",
     "params": {"EqLowGainDb": 0.0, "EqMidGainDb": 0.0, "EqHighGainDb": 0.0,
                "EqLowFreqHz": 200.0, "EqMidFreqHz": 1000.0, "EqHighFreqHz": 6000.0, "EqMidQ": 0.8}}),
    ("eq-02-low-cut-rumble.json", {"chip": 8, "name": "LOW CUT RUMBLE",
     "params": {"EqLowGainDb": -5.0, "EqMidGainDb": -1.5, "EqLowFreqHz": 120.0}}),
    ("eq-03-air-boost.json", {"chip": 8, "name": "AIR BOOST",
     "params": {"EqHighGainDb": 4.0, "EqMidGainDb": 1.0, "EqHighFreqHz": 10000.0}}),
    ("eq-04-mid-scoop.json", {"chip": 8, "name": "MID SCOOP",
     "params": {"EqMidGainDb": -4.0, "EqLowGainDb": 1.5, "EqMidFreqHz": 750.0, "EqMidQ": 1.4}}),
    ("comp-01-bus-glue.json", {"chip": 9, "name": "BUS GLUE", "modePill": "VCA",
     "params": {"CompThresholdDb": -18.0, "CompRatio": 3.0, "CompCharacter": 0, "Mix": 1.0}}),
    ("comp-02-vca-punch.json", {"chip": 9, "name": "VCA PUNCH", "modePill": "VCA",
     "params": {"CompThresholdDb": -14.0, "CompRatio": 4.0, "CompCharacter": 0}}),
    ("comp-03-fet-squeeze.json", {"chip": 9, "name": "FET SQUEEZE", "modePill": "FET",
     "params": {"CompThresholdDb": -22.0, "CompRatio": 6.0, "CompCharacter": 1}}),
    ("lim-01-true-peak.json", {"chip": 10, "name": "TRUE PEAK MAXIMIZER",
     "params": {"LimiterCeilingDb": -0.3, "LimiterLookaheadMs": 5.0, "Mix": 1.0}}),
    ("lim-02-transparent.json", {"chip": 10, "name": "TRANSPARENT CEILING",
     "params": {"LimiterCeilingDb": -1.0, "LimiterReleaseMs": 80.0}}),
    ("lim-03-loud-master.json", {"chip": 10, "name": "LOUD MASTER",
     "params": {"LimiterCeilingDb": -0.1, "LimiterReleaseMs": 40.0}}),
    ("voc-01-talkbox.json", {"chip": 11, "name": "TALKBOX CLASSIC",
     "params": {"VocoderBandCount": 16.0, "VocoderFormant": 0.5, "Mix": 0.85}}),
    ("voc-02-robot.json", {"chip": 11, "name": "ROBOT VOICES",
     "params": {"VocoderBandCount": 12.0, "VocoderFormant": 0.35, "Mix": 0.8}}),
    ("voc-03-formant-morph.json", {"chip": 11, "name": "FORMANT MORPH",
     "params": {"VocoderBandCount": 16.0, "VocoderFormant": 0.72, "Mix": 0.75}}),
]


def main() -> None:
    for filename, payload in PRESETS:
        write(filename, payload)
    print(f"Wrote {len(PRESETS)} design FX presets to {OUT}")


if __name__ == "__main__":
    main()
