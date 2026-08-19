#pragma once

namespace murmur8
{

static const char* previewVertexShader = R"(
    attribute vec2 position;
    varying vec2 vUV;
    void main()
    {
        vUV = position * 0.5 + 0.5;
        gl_Position = vec4 (position, 0.0, 1.0);
    }
)";

static const char* spectralFragmentShader = R"(
    varying vec2 vUV;
    uniform float uTime;
    uniform vec2 uResolution;
    uniform sampler2D uFFTTex;
    uniform float uBinCount;

    vec3 glowColour (float t)
    {
        return mix (vec3 (0.0, 0.82, 0.72), vec3 (0.82, 0.28, 0.62), clamp (t, 0.0, 1.0));
    }

    void main()
    {
        vec3 colour = vec3 (0.02, 0.03, 0.05);
        float x = vUV.x;
        float mag = texture2D (uFFTTex, vec2 (x, 0.5)).r;
        float barH = clamp (mag * 2.2, 0.0, 1.0);
        float inBar = step (1.0 - vUV.y, barH);
        float drift = 0.02 * sin (uTime * 0.5 + x * 20.0);
        colour += glowColour (mag) * inBar;
        colour += drift * glowColour (0.5) * 0.08;
        gl_FragColor = vec4 (colour, 1.0);
    }
)";

static const char* lfoFragmentShader = R"(
    varying vec2 vUV;
    uniform float uTime;
    uniform float uRateHz;
    uniform float uWaveform;
    uniform float uPhase;

    vec3 glowColour (float t) { return mix (vec3 (0.0, 0.78, 0.82), vec3 (0.62, 0.28, 0.95), t); }
    float glow (float d) { return smoothstep (0.02, 0.0, d); }

    float lfoSample (float t, float wf)
    {
        float ph = t * 6.28318 + uPhase;
        if (wf < 0.5) return sin (ph);
        if (wf < 1.5) return 1.0 - 4.0 * abs (fract (t + uPhase / 6.28318) - 0.5);
        if (wf < 2.5) return 2.0 * fract (t + uPhase / 6.28318) - 1.0;
        if (wf < 3.5) return step (0.5, fract (t + uPhase / 6.28318)) * 2.0 - 1.0;
        return fract (sin (t * 127.1) * 43758.5453) * 2.0 - 1.0;
    }

    void main()
    {
        vec3 colour = vec3 (0.02, 0.03, 0.05);
        float t = vUV.x + uTime * uRateHz * 0.15;
        float y = lfoSample (t, uWaveform) * 0.38 + 0.5;
        colour += glowColour (abs (y - 0.5) * 2.0) * glow (abs (vUV.y - y)) * 0.95;
        gl_FragColor = vec4 (colour, 1.0);
    }
)";

static const char* filterFragmentShader = R"(
    varying vec2 vUV;
    uniform float uMode;
    uniform float uCutoff;
    uniform float uResonance;

    vec3 glowColour (float t) { return mix (vec3 (0.0, 0.78, 0.82), vec3 (0.95, 0.55, 0.18), t); }
    float glow (float d) { return smoothstep (0.018, 0.0, d); }

    float response (float fn)
    {
        float q = 0.5 + uResonance * 4.5;
        float d = fn - uCutoff;
        float bell = exp (-d * d * q * 18.0) * uResonance * 1.8;
        int mode = int (uMode + 0.5);
        if (mode == 0) return 1.0 - max (0.0, fn - uCutoff) * (2.5 - uResonance) + bell;
        if (mode == 1) return 1.0 - max (0.0, uCutoff - fn) * (2.5 - uResonance) + bell;
        if (mode == 2) return bell * 2.2 - abs (d) * 3.5;
        if (mode == 3) return 1.0 - bell * 2.5 - abs (d) * 0.5;
        return bell * 2.0;
    }

    void main()
    {
        vec3 colour = vec3 (0.02, 0.03, 0.05);
        float fn = vUV.x;
        float y = response (fn) * 0.35 + 0.5;
        colour += glowColour (fn) * glow (abs (vUV.y - y)) * 0.9;
        float cd = abs (vUV.x - uCutoff);
        colour += vec3 (0.0, 0.9, 0.75) * glow (cd) * 0.35;
        gl_FragColor = vec4 (colour, 1.0);
    }
)";

static const char* envelopeCurveFragmentShader = R"(
    varying vec2 vUV;
    uniform float uTime;
    uniform vec4 uADSR;      // attack, decay, sustain, release (normalized 0-1)
    uniform float uEnvLevel;
    uniform float uEnvProgress;

    vec3 glowColour (float t) { return mix (vec3 (0.0, 0.78, 0.82), vec3 (0.95, 0.55, 0.18), t); }
    float glow (float d) { return smoothstep (0.015, 0.0, d); }

    float envY (float x)
    {
        float a = max (uADSR.x, 0.05);
        float d = max (uADSR.y, 0.05);
        float s = uADSR.z;
        float r = max (uADSR.w, 0.05);
        float t = a + d + 0.35 + r;
        float nx = x * t;
        if (nx < a) return nx / a;
        if (nx < a + d) return 1.0 - (1.0 - s) * ((nx - a) / d);
        if (nx < a + d + 0.35) return s;
        return s * (1.0 - (nx - a - d - 0.35) / r);
    }

    void main()
    {
        vec3 colour = vec3 (0.02, 0.03, 0.05);
        float y = envY (vUV.x) * 0.8 + 0.1;
        colour += glowColour (y) * glow (abs (vUV.y - y)) * 0.92;
        float hx = uEnvProgress;
        float hy = envY (hx) * 0.8 + 0.1;
        colour += glowColour (uEnvLevel) * glow (distance (vUV, vec2 (hx, hy))) * 1.4;
        gl_FragColor = vec4 (colour, 1.0);
    }
)";

static const char* wavetableFragmentShader = R"(
    varying vec2 vUV;
    uniform float uTime;
    uniform float uMorph;
    uniform sampler2D uWaveTex;

    vec3 glowColour (float t) { return mix (vec3 (0.0, 0.78, 0.82), vec3 (0.82, 0.28, 0.62), t); }
    float glow (float d) { return smoothstep (0.014, 0.0, d); }

    void main()
    {
        vec3 colour = vec3 (0.02, 0.03, 0.05);
        float sampleY = texture2D (uWaveTex, vec2 (vUV.x, 0.5)).r * 0.42 + 0.5;
        sampleY += 0.02 * sin (uTime * 2.0 + vUV.x * 12.0) * uMorph;
        colour += glowColour (uMorph) * glow (abs (vUV.y - sampleY)) * 0.95;
        gl_FragColor = vec4 (colour, 1.0);
    }
)";

static const char* granularFragmentShader = R"(
    varying vec2 vUV;
    uniform float uTime;
    uniform float uDensity;
    uniform float uMix;

    vec3 glowColour (float t) { return mix (vec3 (0.0, 0.78, 0.82), vec3 (0.95, 0.55, 0.18), t); }
    float hash (vec2 p) { return fract (sin (dot (p, vec2 (127.1, 311.7))) * 43758.5453); }
    float glow (float d, float r) { return smoothstep (r + 0.02, r, d); }

    void main()
    {
        vec3 colour = vec3 (0.02, 0.03, 0.05);
        for (int i = 0; i < 32; i++)
        {
            vec2 seed = vec2 (float (i) * 3.7, float (i) * 2.3);
            float life = fract (hash (seed) + uTime * (0.15 + uDensity));
            vec2 pos = vec2 (hash (seed + 1.0), hash (seed + 2.0));
            pos.x += sin (uTime + float (i)) * 0.03;
            float d = distance (vUV, pos);
            colour += glowColour (life) * glow (d, 0.02) * (1.0 - life) * uMix;
        }
        gl_FragColor = vec4 (colour, 1.0);
    }
)";

static const char* vuMeterFragmentShader = R"(
    varying vec2 vUV;
    uniform float uPeakL;
    uniform float uPeakR;
    uniform float uRmsL;
    uniform float uRmsR;
    uniform float uVertical;

    vec3 barColour (float level)
    {
        return mix (vec3 (0.0, 0.85, 0.65), vec3 (0.95, 0.25, 0.25), smoothstep (0.7, 1.0, level));
    }

    void main()
    {
        vec3 colour = vec3 (0.02, 0.03, 0.05);
        if (uVertical > 0.5)
        {
            float barX = step (0.48, vUV.x);
            float peak = mix (uPeakL, uPeakR, barX);
            float rms = mix (uRmsL, uRmsR, barX);
            float bx = barX > 0.5 ? 0.58 : 0.12;
            if (vUV.x > bx && vUV.x < bx + 0.28 && vUV.y > 1.0 - rms)
                colour += barColour (rms) * 0.65;
            colour += vec3 (1.0) * step (abs (vUV.y - (1.0 - peak)), 0.008) * 0.8;
        }
        else
        {
            float barY = step (0.5, vUV.y);
            float peak = mix (uPeakL, uPeakR, barY);
            float rms = mix (uRmsL, uRmsR, barY);
            float by = barY > 0.5 ? 0.12 : 0.58;
            if (vUV.y > by && vUV.y < by + 0.28 && vUV.x < rms)
                colour += barColour (rms) * 0.65;
            colour += vec3 (1.0) * step (abs (vUV.x - peak), 0.008) * 0.8;
        }
        gl_FragColor = vec4 (colour, 1.0);
    }
)";

} // namespace murmur8
