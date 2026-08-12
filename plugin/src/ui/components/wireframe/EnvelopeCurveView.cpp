#include "EnvelopeCurveView.h"

#include "../../theme/ObsidianFonts.h"
#include "../../theme/ObsidianPalette.h"
#include "WireframeProjection.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui::wireframe
{
    namespace
    {
        [[nodiscard]] float loadEnvParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id,
                                          float fallback = 0.0f)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        }

        void appendCurvedSegment(juce::Path& path, float x0, float x1, float y0, float y1, float curveShape,
                                  bool shapeUp, int steps)
        {
            path.lineTo(x0, y0);
            for (int i = 1; i <= steps; ++i)
            {
                const float t = static_cast<float>(i) / static_cast<float>(steps);
                const float shaped = shapeUp ? envelopeShapeUp(t, curveShape) : envelopeShapeDown(t, curveShape);
                const float y = y0 + (y1 - y0) * shaped;
                const float x = x0 + (x1 - x0) * t;
                path.lineTo(x, y);
            }
        }

        void buildEnvelopePath(const EnvelopePreviewParams& params, juce::Rectangle<float> bounds, juce::Path& outline,
                                juce::Path& fillPath, float& sustainEndX)
        {
            const float minStage = 0.06f;
            const float sustainDisplay = juce::jmax(0.18f, minStage);
            const float delayW = params.delaySeconds > 0.0f ? juce::jmax(minStage, params.delaySeconds) : 0.0f;
            const float attackW = juce::jmax(minStage, params.attackSeconds);
            const float holdW = params.holdSeconds > 0.0f ? juce::jmax(minStage, params.holdSeconds) : 0.0f;
            const float decayW = juce::jmax(minStage, params.decaySeconds);
            const float releaseW = juce::jmax(minStage, params.releaseSeconds);
            const float totalW = delayW + attackW + holdW + decayW + sustainDisplay + releaseW;

            auto xFor = [&](float stageStart) { return bounds.getX() + (stageStart / totalW) * bounds.getWidth(); };
            auto yFor = [&](float level)
            {
                return bounds.getBottom() - level * bounds.getHeight() * 0.88f - bounds.getHeight() * 0.06f;
            };

            outline.clear();
            fillPath.clear();

            float cursor = 0.0f;
            const float yZero = yFor(0.0f);
            outline.startNewSubPath(xFor(cursor), yZero);

            if (delayW > 0.0f)
            {
                const float x1 = xFor(cursor + delayW);
                outline.lineTo(x1, yZero);
                cursor += delayW;
            }

            {
                const float x0 = xFor(cursor);
                const float x1 = xFor(cursor + attackW);
                appendCurvedSegment(outline, x0, x1, yFor(0.0f), yFor(1.0f), params.curveShape, true, 32);
                cursor += attackW;
            }

            if (holdW > 0.0f)
            {
                const float x1 = xFor(cursor + holdW);
                outline.lineTo(x1, yFor(1.0f));
                cursor += holdW;
            }

            {
                const float x0 = xFor(cursor);
                const float x1 = xFor(cursor + decayW);
                appendCurvedSegment(outline, x0, x1, yFor(1.0f), yFor(params.sustainLevel), params.curveShape, false,
                                    32);
                cursor += decayW;
            }

            sustainEndX = xFor(cursor + sustainDisplay);
            outline.lineTo(sustainEndX, yFor(params.sustainLevel));

            {
                const float x0 = sustainEndX;
                const float x1 = xFor(cursor + sustainDisplay + releaseW);
                appendCurvedSegment(outline, x0, x1, yFor(params.sustainLevel), yFor(0.0f), params.curveShape, false,
                                    32);
            }

            fillPath = outline;
            fillPath.lineTo(outline.getBounds().getRight(), bounds.getBottom());
            fillPath.lineTo(bounds.getX(), bounds.getBottom());
            fillPath.closeSubPath();
        }
    } // namespace

    EnvelopeCurveView::EnvelopeCurveView(juce::AudioProcessorValueTreeState& apvts, std::size_t envIndex)
        : apvts_(apvts), envIndex_(envIndex)
    {
        setCaption("AMP ENVELOPE · DAHDSR");
        setSubCaption("Release tail shown after sustain plateau");
        startTimerHz(8);
        refreshParams();
    }

    EnvelopeCurveView::~EnvelopeCurveView() { stopTimer(); }

    void EnvelopeCurveView::timerCallback()
    {
        refreshParams();
        repaint();
    }

    void EnvelopeCurveView::refreshParams()
    {
        const auto prefix = envelopeParamId(envIndex_, "");
        params_.delaySeconds = loadEnvParam(apvts_, prefix + "Delay");
        params_.attackSeconds = loadEnvParam(apvts_, prefix + "Attack", 0.005f);
        params_.holdSeconds = loadEnvParam(apvts_, prefix + "Hold");
        params_.decaySeconds = loadEnvParam(apvts_, prefix + "Decay", 0.2f);
        params_.sustainLevel = loadEnvParam(apvts_, prefix + "Sustain", 0.7f);
        params_.releaseSeconds = loadEnvParam(apvts_, prefix + "Release", 0.3f);
        params_.curveShape = loadEnvParam(apvts_, prefix + "Curve", 2.0f);
    }

    void EnvelopeCurveView::paint(juce::Graphics& g)
    {
        auto bounds = meshBounds();
        paintSubCaption(g, bounds);

        juce::Path outline;
        juce::Path fillPath;
        float sustainEndX = bounds.getX();
        buildEnvelopePath(params_, bounds, outline, fillPath, sustainEndX);

        g.setColour(palette::kAccent.withAlpha(0.10f));
        g.fillPath(fillPath);
        strokeGlowPath(g, outline, 1.0f, 2.0f, true);

        g.setColour(palette::kBorderBright.withAlpha(0.55f));
        g.drawHorizontalLine(static_cast<int>(bounds.getBottom() - bounds.getHeight() * 0.06f),
                             bounds.getX(), bounds.getRight());

        const float markerY = bounds.getBottom() + 2.0f;
        g.setFont(fonts::label(8.5f));
        g.setColour(palette::kTextDim);

        const char* labels[] = {"D", "A", "H", "D", "S", "R"};
        const float stageStarts[] = {0.0f, 0.06f, 0.18f, 0.24f, 0.36f, 0.54f};
        for (int i = 0; i < 6; ++i)
        {
            const float x = bounds.getX() + stageStarts[i] * bounds.getWidth();
            g.drawText(labels[i], juce::Rectangle<float>(x - 8.0f, markerY, 16.0f, 12.0f),
                       juce::Justification::centred);
        }

        g.setColour(palette::kAccentWarm.withAlpha(0.85f));
        g.drawVerticalLine(static_cast<int>(sustainEndX), bounds.getY(), bounds.getBottom());
        g.setFont(fonts::label(8.0f));
        g.drawText("note off", juce::Rectangle<float>(sustainEndX + 3.0f, bounds.getY(), 48.0f, 12.0f),
                   juce::Justification::centredLeft);
    }

} // namespace pw8::plugin::ui::wireframe
