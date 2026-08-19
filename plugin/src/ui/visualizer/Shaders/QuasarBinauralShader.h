#pragma once

namespace murmur8
{

static const char* quasarBinauralVertexShader = R"(
    attribute vec2 position;
    varying vec2 vUV;

    void main()
    {
        vUV = position * 0.5 + 0.5;
        gl_Position = vec4 (position, 0.0, 1.0);
    }
)";

static const char* quasarBinauralFragmentShader = R"(
    varying vec2 vUV;

    uniform float uTime;
    uniform vec2  uResolution;

    uniform vec4  uQsr1;  // angleDeg, distance, height, level
    uniform vec4  uQsr2;
    uniform float uCrossfeed;
    uniform float uWidth;
    uniform float uMix;
    uniform float uPeakL;
    uniform float uPeakR;

    vec3 glowColour (float t)
    {
        vec3 cyan = vec3 (0.0, 0.78, 0.82);
        vec3 violet = vec3 (0.62, 0.28, 0.95);
        vec3 amber = vec3 (0.95, 0.55, 0.18);
        return mix (mix (cyan, violet, t), amber, 0.25);
    }

    float glow (float dist, float radius, float softness)
    {
        return smoothstep (radius + softness, radius, dist);
    }

    vec2 sourcePos (vec4 src, vec2 centre, float ringR)
    {
        float az = radians (src.x - 90.0);
        float radiusNorm = mix (0.22, 1.0, clamp (src.y, 0.0, 1.0));
        float r = ringR * radiusNorm;
        float elev = -src.z * ringR * 0.18;
        return centre + vec2 (cos (az), sin (az)) * r + vec2 (0.0, elev);
    }

    void main()
    {
        vec2 uv = vUV;
        vec2 centre = vec2 (0.5, 0.5);
        vec3 colour = vec3 (0.015, 0.02, 0.04);

        float aspect = uResolution.x / max (uResolution.y, 1.0);
        vec2 p = (uv - centre);
        p.x *= aspect;
        float ringR = 0.36;

        float ambient = 0.012 * sin (uTime * 1.1 + uv.x * 8.0) * (uPeakL + uPeakR);
        colour += ambient;

        for (int i = 0; i < 3; i++)
        {
            float ri = ringR * (0.55 + float (i) * 0.225);
            float d = abs (length (p) - ri);
            colour += vec3 (0.0, 0.5, 0.55) * glow (d, 0.0, 0.004) * (0.35 - float (i) * 0.08);
        }

        for (int i = 0; i < 4; i++)
        {
            float ang = radians (float (i) * 90.0 - 90.0);
            vec2 tick = centre + vec2 (cos (ang), sin (ang)) * ringR * vec2 (1.0 / aspect, 1.0);
            vec2 from = centre;
            float lineD = length (cross (vec3 (tick - from, 0.0), vec3 (uv - from, 0.0))) / length (tick - from);
            colour += vec3 (0.0, 0.45, 0.5) * glow (lineD, 0.0, 0.003) * 0.25;
        }

        vec2 head = centre;
        float headDist = length (p);
        colour += glowColour (0.5) * glow (headDist, 0.04, 0.02) * 0.9;

        vec2 s1 = sourcePos (uQsr1, centre, ringR);
        s1.x /= aspect;
        vec2 s2 = sourcePos (uQsr2, centre, ringR);
        s2.x /= aspect;

        vec2 tether1 = uv - s1;
        vec2 tether2 = uv - s2;
        colour += glowColour (0.15) * glow (length (tether1), 0.0, 0.008) * 0.35;
        colour += glowColour (0.85) * glow (length (tether2), 0.0, 0.008) * 0.35;

        float crossArc = abs (uv.y - 0.5) + abs (uv.x - 0.5) * 0.3;
        colour += vec3 (0.2, 0.6, 0.55) * glow (crossArc, 0.15 + uCrossfeed * 0.25, 0.04) * uCrossfeed * 0.4;

        float wobble = sin (uTime * 2.0 + uQsr1.x * 0.05) * uWidth * 0.02;
        colour += glowColour (0.2) * glow (length (uv - s1 - vec2 (wobble, 0.0)), 0.0, 0.015) * uQsr1.w * 1.4;
        colour += glowColour (0.8) * glow (length (uv - s2), 0.0, 0.015) * uQsr2.w * 1.4;

        for (int i = 0; i < 16; i++)
        {
            float t = float (i) / 15.0;
            float ang1 = radians (uQsr1.x - 90.0 + sin (uTime + t * 6.28) * 8.0);
            vec2 trail1 = centre + vec2 (cos (ang1), sin (ang1)) * ringR * mix (0.22, 1.0, uQsr1.y) * vec2 (1.0 / aspect, 1.0);
            colour += glowColour (0.25) * glow (length (uv - trail1), 0.0, 0.012) * (1.0 - t) * 0.12;
        }

        gl_FragColor = vec4 (colour, 1.0);
    }
)";

} // namespace murmur8
