#pragma once

namespace murmur8
{

static const char* waveformVertexShader = R"(
    attribute vec2 position;
    varying vec2 vUV;

    void main()
    {
        vUV = position * 0.5 + 0.5;
        gl_Position = vec4 (position, 0.0, 1.0);
    }
)";

static const char* waveformFragmentShader = R"(
    varying vec2 vUV;

    uniform sampler2D uWaveformTex;
    uniform float uWaveformColumns;
    uniform vec2  uResolution;

    vec3 glowColour (float t)
    {
        vec3 a = vec3 (0.10, 0.65, 0.90);
        vec3 b = vec3 (0.85, 0.25, 0.65);
        return mix (a, b, t);
    }

    float glow (float dist, float radius, float softness)
    {
        return smoothstep (radius + softness, radius, dist);
    }

    void main()
    {
        vec2 uv = vUV;
        vec3 colour = vec3 (0.02, 0.02, 0.04);

        float x = uv.x;
        vec2 minMax = texture2D (uWaveformTex, vec2 (x, 0.5)).rg;
        float sampleY = (minMax.r + minMax.g) * 0.5;
        float y = sampleY * 0.42 + 0.5;
        float ribbonHalf = abs (minMax.g - minMax.r) * 0.42;
        float d = abs (uv.y - y);
        float inRibbon = step (y - ribbonHalf, uv.y) * step (uv.y, y + ribbonHalf);
        colour += glowColour (abs (minMax.g - minMax.r)) * glow (d, 0.0, 0.012) * inRibbon;

        gl_FragColor = vec4 (colour, 1.0);
    }
)";

} // namespace murmur8
