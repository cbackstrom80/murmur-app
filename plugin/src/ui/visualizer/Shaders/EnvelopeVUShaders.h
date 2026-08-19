/*
    EnvelopeVUShaders.h

    First working visual module: envelope playhead + stereo VU meter,
    rendered in ONE draw call via a single fullscreen-quad fragment shader.

    Everything that "moves" - the envelope glow dot, the VU needle
    ballistics, the ambient background pulse - is computed here on the
    GPU from a handful of uniforms. The CPU side only ever uploads:
      - uTime               (float, seconds)
      - uEnvLevel/uEnvStage/uEnvProgress
      - uPeakL/uPeakR/uRmsL/uRmsR

    That's 8 floats a frame. No per-frame CPU path rebuilding.
*/

#pragma once

namespace murmur8
{

static const char* envelopeVuVertexShader = R"(
    attribute vec2 position;
    varying vec2 vUV;

    void main()
    {
        vUV = position * 0.5 + 0.5;
        gl_Position = vec4 (position, 0.0, 1.0);
    }
)";

static const char* envelopeVuFragmentShader = R"(
    varying vec2 vUV;

    uniform float uTime;
    uniform vec2  uResolution;

    // Envelope
    uniform float uEnvLevel;      // 0-1 current output
    uniform float uEnvStage;      // 0=idle 1=attack 2=decay 3=sustain 4=release
    uniform float uEnvProgress;   // 0-1 through current stage

    // VU (already smoothed on CPU per-block is fine; ballistics glow is GPU-side)
    uniform float uPeakL;
    uniform float uPeakR;
    uniform float uRmsL;
    uniform float uRmsR;
    uniform float uEnvelopeOnly;  // 1 = envelope fills width, 0 = split with VU column

    // Shared visual identity - reuse this palette across every module so the
    // whole plugin reads as one instrument, not eight bolted-on widgets.
    vec3 glowColour (float t)
    {
        vec3 a = vec3 (0.10, 0.65, 0.90); // cool cyan
        vec3 b = vec3 (0.85, 0.25, 0.65); // warm magenta
        return mix (a, b, t);
    }

    // Cheap single-pass glow: instead of stroking a path 3-4x on the CPU,
    // exponentiate the distance falloff in the fragment shader. One draw call.
    float glow (float dist, float radius, float softness)
    {
        return smoothstep (radius + softness, radius, dist);
    }

    // Analytic ADSR curve shape for the envelope trace - no per-sample path data,
    // just evaluate the curve at this pixel's x-position.
    float envelopeCurveY (float x)
    {
        // four normalized segments across the width, shape only (illustrative -
        // swap in your actual ADSR time ratios via additional uniforms if you
        // want the trace to reflect real attack/decay/release times)
        if (x < 0.25) return x / 0.25;
        if (x < 0.45) return 1.0 - 0.3 * ((x - 0.25) / 0.20);
        if (x < 0.80) return 0.7;
        return 0.7 * (1.0 - (x - 0.80) / 0.20);
    }

    void main()
    {
        vec2 uv = vUV;
        vec3 colour = vec3 (0.02, 0.02, 0.04); // dark base field

        // --- Ambient background pulse tied to overall level, cheap sine, GPU-only ---
        float ambient = 0.015 * sin (uTime * 1.3 + uv.x * 6.0) * (uPeakL + uPeakR);
        colour += ambient;

        float envWidth = uEnvelopeOnly > 0.5 ? 1.0 : 0.6;

        // --- Envelope trace ---
        if (uv.x < envWidth)
        {
            float ex = uv.x / envWidth;
            float curveY = envelopeCurveY (ex) * 0.8 + 0.1;
            float d = abs (uv.y - curveY);
            colour += glowColour (curveY) * glow (d, 0.0, 0.01) * 0.9;

            float headX = uEnvProgress * envWidth;
            float headY = envelopeCurveY (headX / envWidth) * 0.8 + 0.1;
            float headDist = distance (uv, vec2 (headX, headY));
            colour += glowColour (uEnvLevel) * glow (headDist, 0.008, 0.02) * 1.5;
        }

        // --- VU meters (right column, hidden in envelope-only layout) ---
        if (uEnvelopeOnly < 0.5 && uv.x > 0.65)
        {
            float barX = (uv.x - 0.65) / 0.35;
            float which = step (0.5, barX); // 0 = left channel bar, 1 = right
            float peak = mix (uPeakL, uPeakR, which);
            float rms  = mix (uRmsL,  uRmsR,  which);

            float barFill = step (uv.y, rms);
            float peakLine = glow (abs (uv.y - peak), 0.0, 0.01);

            vec3 barColour = mix (vec3 (0.1, 0.9, 0.5), vec3 (0.95, 0.2, 0.2), smoothstep (0.7, 1.0, uv.y));
            colour += barColour * barFill * 0.6;
            colour += vec3 (1.0) * peakLine;
        }

        gl_FragColor = vec4 (colour, 1.0);
    }
)";

} // namespace murmur8
