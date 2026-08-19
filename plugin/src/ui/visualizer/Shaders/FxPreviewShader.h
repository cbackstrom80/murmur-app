#pragma once

namespace murmur8
{

static const char* fxPreviewVertexShader = R"(
    attribute vec2 position;
    varying vec2 vUV;

    void main()
    {
        vUV = position * 0.5 + 0.5;
        gl_Position = vec4 (position, 0.0, 1.0);
    }
)";

// uFxKind: 0=bypass 1=sat 2=chorus 3=delay 4=mood 5=freqshift 6=fractal 7=reverb
//          8=eq 9=comp 10=limiter 11=vocoder 12=granular
static const char* fxPreviewFragmentShader = R"(
    varying vec2 vUV;

    uniform float uTime;
    uniform vec2  uResolution;
    uniform float uFxKind;
    uniform float uMix;
    uniform vec4  uParamsA;  // drive, rate, depth, feedback
    uniform vec4  uParamsB;  // decay, size, damping, morph
    uniform vec4  uParamsC;  // threshold, ratio, ceiling, bands

    vec3 glowColour (float t)
    {
        vec3 a = vec3 (0.0, 0.82, 0.72);
        vec3 b = vec3 (0.82, 0.28, 0.62);
        return mix (a, b, clamp (t, 0.0, 1.0));
    }

    float glow (float dist, float radius, float softness)
    {
        return smoothstep (radius + softness, radius, dist);
    }

    float hash (vec2 p) { return fract (sin (dot (p, vec2 (12.9898, 78.233))) * 43758.5453); }

    float satCurve (float x, float drive)
    {
        float k = 1.0 + drive * 0.35;
        return tanh (x * k) / tanh (k);
    }

    void main()
    {
        vec2 uv = vUV;
        vec3 colour = vec3 (0.02, 0.03, 0.05);
        int kind = int (uFxKind + 0.5);

        if (kind == 0)
        {
            float d = abs (uv.y - 0.5);
            colour += glowColour (0.2) * glow (d, 0.0, 0.012) * 0.8;
        }
        else if (kind == 1)
        {
            float drive = uParamsA.x;
            float x = uv.x * 2.0 - 1.0;
            float y = satCurve (x, drive) * 0.42 + 0.5;
            float d = abs (uv.y - y);
            colour += glowColour (abs (y - 0.5) * 2.0) * glow (d, 0.0, 0.014) * 0.95;
        }
        else if (kind == 2)
        {
            float rate = max (uParamsA.y, 0.05);
            float depth = uParamsA.z * 0.001 + 0.08;
            int voices = 3;
            for (int v = 0; v < 4; v++)
            {
                if (v >= voices) break;
                float phase = float (v) * 1.5707963;
                float y = 0.5 + depth * sin (uTime * rate * 6.28318 + phase + uv.x * 8.0) * 3.0;
                float d = abs (uv.y - y);
                colour += glowColour (float (v) / 3.0) * glow (d, 0.0, 0.01) * 0.85;
            }
        }
        else if (kind == 3)
        {
            vec2 centered = uv - 0.5;
            float dist = length (centered);
            float delayMs = max (uParamsA.w, 1.0);
            float feedback = clamp (uParamsA.z, 0.0, 0.98);
            float ringSpacing = 0.04 + delayMs * 0.00015;
            float ringPhase = fract (dist / ringSpacing - uTime * 0.35);
            float ring = glow (abs (ringPhase - 0.5) * ringSpacing, 0.0, 0.012);
            float fade = pow (feedback, floor (dist / ringSpacing));
            colour += glowColour (uMix) * ring * fade * 1.1;
            float wobble = sin (uTime * uParamsA.y + dist * 12.0) * uParamsB.w * 0.02;
            float wave = abs (uv.y - 0.5 - wobble);
            colour += glowColour (0.4) * glow (wave, 0.0, 0.008) * 0.5;
        }
        else if (kind == 4)
        {
            float t = uv.x;
            float shelf = mix (uParamsB.x, uParamsB.y, smoothstep (0.2, 0.5, t));
            shelf = mix (shelf, uParamsB.z, smoothstep (0.5, 0.85, t));
            float y = shelf * 0.25 + 0.5;
            float d = abs (uv.y - y);
            colour += glowColour (t) * glow (d, 0.0, 0.012) * 0.9;
        }
        else if (kind == 5)
        {
            float turns = 2.0 + uParamsA.x * 0.05;
            float ang = atan (uv.y - 0.5, uv.x - 0.5);
            float rad = length (uv - 0.5);
            float spiral = fract (ang / 6.28318 * turns + rad * 4.0 - uTime * 0.2);
            colour += glowColour (spiral) * glow (abs (spiral - 0.5), 0.0, 0.02) * uParamsA.y;
        }
        else if (kind == 6)
        {
            for (int i = 0; i < 20; i++)
            {
                vec2 seed = vec2 (float (i) * 2.17, float (i) * 1.31);
                vec2 pos = vec2 (hash (seed), hash (seed + 1.0));
                pos += 0.05 * sin (uTime * 0.7 + float (i));
                float d = distance (uv, pos);
                colour += glowColour (hash (seed + 2.0)) * glow (d, 0.0, 0.025) * (0.4 + uParamsB.w);
            }
        }
        else if (kind == 7)
        {
            float decay = max (uParamsB.x, 0.1);
            for (int i = 0; i < 28; i++)
            {
                vec2 seed = vec2 (float (i) * 7.13, float (i) * 3.71);
                float birth = hash (seed) * decay;
                float age = mod (uTime - birth, decay) / decay;
                vec2 pos = vec2 (hash (seed + 1.0), hash (seed + 2.0));
                float d = distance (uv, pos);
                colour += glowColour (age) * glow (d, 0.0, 0.03) * (1.0 - age) * uMix;
            }
            float tail = exp (-uv.x * (2.0 / max (decay, 0.2)));
            colour += glowColour (0.6) * tail * glow (abs (uv.y - 0.5), 0.0, 0.05) * 0.4;
        }
        else if (kind == 8)
        {
            float lowDb = uParamsA.x;
            float midDb = uParamsA.y;
            float highDb = uParamsA.z;
            float midQ = max (uParamsA.w, 0.15);
            float lowT = uParamsB.x;
            float midT = uParamsB.y;
            float highT = uParamsB.z;
            float t = uv.x;
            float lowG = pow (10.0, lowDb / 20.0) - 1.0;
            float midG = pow (10.0, midDb / 20.0) - 1.0;
            float highG = pow (10.0, highDb / 20.0) - 1.0;
            float midW = clamp (0.06 / midQ, 0.025, 0.18);
            float l = lowG * exp (-pow ((t - lowT) / 0.10, 2.0));
            float m = midG * exp (-pow ((t - midT) / midW, 2.0));
            float h = highG * exp (-pow ((t - highT) / 0.12, 2.0));
            float y = (l + m + h) * 0.55 * 0.42 + 0.5;
            float d = abs (uv.y - y);
            colour += glowColour (t) * glow (d, 0.0, 0.014) * 0.95;
        }
        else if (kind == 9)
        {
            float thresh = (uParamsC.x + 24.0) / 48.0;
            float ratio = max (uParamsC.y, 1.0);
            float x = uv.x;
            float y = x;
            if (x > thresh)
                y = thresh + (x - thresh) / ratio;
            y = y * 0.8 + 0.1;
            float d = abs (uv.y - y);
            colour += glowColour (0.35) * glow (d, 0.0, 0.012) * 0.9;
            float gr = clamp (-uParamsC.z / 24.0, 0.0, 1.0);
            colour += vec3 (0.1, 0.5, 0.45) * step (uv.x, gr) * step (0.08, uv.y) * step (uv.y, 0.14) * 0.7;
        }
        else if (kind == 10)
        {
            float ceilY = (uParamsC.w + 6.0) / 12.0;
            float d = abs (uv.y - ceilY);
            colour += glowColour (0.7) * glow (d, 0.0, 0.008) * 1.2;
            float peak = 0.5 + 0.35 * sin (uTime * 3.0);
            colour += vec3 (1.0, 0.3, 0.35) * glow (abs (uv.y - peak), 0.0, 0.006) * 0.8;
        }
        else if (kind == 11)
        {
            int bands = int (uParamsC.w + 0.5);
            bands = max (bands, 8);
            float bw = 1.0 / float (bands);
            for (int b = 0; b < 32; b++)
            {
                if (b >= bands) break;
                float x0 = float (b) * bw;
                float h = 0.15 + 0.55 * abs (sin (float (b) * 0.7 + uTime * 2.0));
                if (uv.x >= x0 && uv.x <= x0 + bw * 0.85 && uv.y <= h)
                    colour += glowColour (float (b) / float (bands)) * 0.55;
            }
        }
        else if (kind == 12)
        {
            for (int i = 0; i < 24; i++)
            {
                vec2 seed = vec2 (float (i) * 3.3, float (i) * 5.1);
                float life = fract (hash (seed) + uTime * (0.2 + uParamsA.y));
                vec2 pos = vec2 (hash (seed + 1.0), hash (seed + 2.0));
                pos.x += sin (uTime + float (i)) * 0.04;
                float rx = 0.02 + uParamsA.z * 0.0003;
                float ry = 0.01 + uParamsB.w * 0.0005;
                vec2 d = (uv - pos) / vec2 (rx, ry);
                float ell = length (d);
                colour += glowColour (life) * glow (ell, 0.5, 0.15) * (1.0 - life) * uMix;
            }
        }

        gl_FragColor = vec4 (colour, 1.0);
    }
)";

} // namespace murmur8
