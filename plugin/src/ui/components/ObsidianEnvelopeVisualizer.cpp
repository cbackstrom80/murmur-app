#include "ObsidianEnvelopeVisualizer.h"

#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] juce::RangedAudioParameter* asRangedParam(juce::AudioProcessorValueTreeState& apvts,
                                                                const juce::String& id)
        {
            return dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(id));
        }
    } // namespace

    ObsidianEnvelopeVisualizer::ObsidianEnvelopeVisualizer(juce::AudioProcessorValueTreeState& apvts,
                                                             std::size_t envIndex)
        : apvts_(apvts), envIndex_(envIndex)
    {
        startTimerHz(15);
        refreshFromApvts();
        rebuildLayout();
    }

    ObsidianEnvelopeVisualizer::~ObsidianEnvelopeVisualizer()
    {
        stopTimer();
        endActiveGestures();
    }

    juce::String ObsidianEnvelopeVisualizer::paramId(const char* suffix) const
    {
        return envelopeParamId(envIndex_, suffix);
    }

    float ObsidianEnvelopeVisualizer::readParam(const char* suffix, float fallback) const
    {
        if (auto* raw = apvts_.getRawParameterValue(paramId(suffix)))
            return raw->load();
        return fallback;
    }

    void ObsidianEnvelopeVisualizer::refreshFromApvts()
    {
        params_.delaySeconds = readParam("Delay", 0.0f);
        params_.attackSeconds = readParam("Attack", 0.005f);
        params_.holdSeconds = readParam("Hold", 0.0f);
        params_.decaySeconds = readParam("Decay", 0.2f);
        params_.sustainLevel = readParam("Sustain", 0.7f);
        params_.releaseSeconds = readParam("Release", 0.3f);
        params_.curveShape = readParam("Curve", 2.0f);
    }

    juce::Rectangle<float> ObsidianEnvelopeVisualizer::graphBounds() const
    {
        return getLocalBounds().toFloat().reduced(static_cast<float>(kMargin));
    }

    void ObsidianEnvelopeVisualizer::rebuildLayout()
    {
        layout_ = wireframe::computeEnvelopeLayout(params_, graphBounds());
    }

    juce::Point<float> ObsidianEnvelopeVisualizer::handlePosition(HandleKind kind) const
    {
        switch (kind)
        {
            case HandleKind::DelayEnd:
                return {layout_.delayEndX, layout_.baselineY};
            case HandleKind::AttackPeak:
                return {layout_.attackPeakX, layout_.yFromLevel(1.0f)};
            case HandleKind::HoldEnd:
                return {layout_.holdEndX, layout_.yFromLevel(1.0f)};
            case HandleKind::DecaySustain:
                return {layout_.decaySustainX, layout_.decaySustainY};
            case HandleKind::ReleaseEnd:
                return {layout_.releaseEndX, layout_.baselineY};
            default:
                return {};
        }
    }

    ObsidianEnvelopeVisualizer::HandleKind ObsidianEnvelopeVisualizer::handleAtPoint(juce::Point<float> pt) const
    {
        const HandleKind candidates[] = {HandleKind::ReleaseEnd, HandleKind::DecaySustain, HandleKind::HoldEnd,
                                         HandleKind::AttackPeak, HandleKind::DelayEnd};

        for (const auto kind : candidates)
        {
            if (handlePosition(kind).getDistanceFrom(pt) <= kHitRadius)
                return kind;
        }

        return HandleKind::None;
    }

    void ObsidianEnvelopeVisualizer::writeParam(const char* suffix, float realValue)
    {
        static constexpr const char* kFields[] = {"Delay", "Attack", "Hold", "Decay", "Sustain", "Release"};
        int fieldIndex = -1;
        for (int i = 0; i < 6; ++i)
        {
            if (juce::String(kFields[i]) == suffix)
            {
                fieldIndex = i;
                break;
            }
        }

        if (fieldIndex < 0)
            return;

        if (auto* param = asRangedParam(apvts_, paramId(suffix)))
        {
            if (!gestureActive_[static_cast<std::size_t>(fieldIndex)])
            {
                param->beginChangeGesture();
                gestureActive_[static_cast<std::size_t>(fieldIndex)] = true;
            }
            param->setValueNotifyingHost(param->convertTo0to1(realValue));
        }
    }

    void ObsidianEnvelopeVisualizer::endActiveGestures()
    {
        static constexpr const char* kFields[] = {"Delay", "Attack", "Hold", "Decay", "Sustain", "Release"};
        for (int i = 0; i < 6; ++i)
        {
            if (!gestureActive_[static_cast<std::size_t>(i)])
                continue;

            if (auto* param = asRangedParam(apvts_, paramId(kFields[i])))
                param->endChangeGesture();

            gestureActive_[static_cast<std::size_t>(i)] = false;
        }
    }

    void ObsidianEnvelopeVisualizer::applyDrag(HandleKind kind, juce::Point<float> pt)
    {
        const float t = layout_.displayTimeFromX(pt.x);
        const float delayEnd = layout_.delayDisplay;
        const float attackEnd = layout_.delayDisplay + layout_.attackDisplay;
        const float holdEnd = attackEnd + layout_.holdDisplay;
        const float decayEnd = holdEnd + layout_.decayDisplay;
        const float sustainEnd = decayEnd + layout_.sustainDisplay;

        switch (kind)
        {
            case HandleKind::DelayEnd:
            {
                const float clamped = juce::jlimit(0.0f, attackEnd, t);
                writeParam("Delay", wireframe::envelopeTimeFromDisplayWeight(clamped));
                break;
            }
            case HandleKind::AttackPeak:
            {
                const float maxAttackEnd = layout_.holdDisplay > 0.0f
                                               ? holdEnd
                                               : (decayEnd > attackEnd ? decayEnd : attackEnd + wireframe::kEnvelopeMinStage);
                const float clamped = juce::jlimit(delayEnd, maxAttackEnd, t);
                writeParam("Attack",
                           wireframe::envelopeTimeFromDisplayWeight(juce::jmax(0.0f, clamped - delayEnd)));
                break;
            }
            case HandleKind::HoldEnd:
            {
                const float maxHoldEnd = decayEnd > attackEnd ? decayEnd : attackEnd + wireframe::kEnvelopeMinStage;
                const float clamped = juce::jlimit(attackEnd, maxHoldEnd, t);
                writeParam("Hold", wireframe::envelopeTimeFromDisplayWeight(juce::jmax(0.0f, clamped - attackEnd)));
                break;
            }
            case HandleKind::DecaySustain:
            {
                const float clampedX = juce::jlimit(holdEnd, sustainEnd > holdEnd ? sustainEnd : holdEnd + wireframe::kEnvelopeMinStage,
                                                    pt.x);
                const float displayTime = layout_.displayTimeFromX(clampedX);
                writeParam("Decay", wireframe::envelopeTimeFromDisplayWeight(juce::jmax(0.0f, displayTime - holdEnd)));
                writeParam("Sustain", layout_.levelFromY(pt.y));
                break;
            }
            case HandleKind::ReleaseEnd:
            {
                const float clamped = juce::jlimit(sustainEnd, layout_.totalDisplaySeconds, t);
                writeParam("Release", wireframe::envelopeTimeFromDisplayWeight(juce::jmax(0.0f, clamped - sustainEnd)));
                break;
            }
            default:
                break;
        }

        refreshFromApvts();
        rebuildLayout();
    }

    void ObsidianEnvelopeVisualizer::updateHoverCursor(HandleKind kind)
    {
        switch (kind)
        {
            case HandleKind::DecaySustain:
                setMouseCursor(juce::MouseCursor::UpDownLeftRightResizeCursor);
                break;
            case HandleKind::None:
                setMouseCursor(juce::MouseCursor::NormalCursor);
                break;
            default:
                setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
                break;
        }
    }

    void ObsidianEnvelopeVisualizer::timerCallback()
    {
        if (dragging_)
            return;

        refreshFromApvts();
        rebuildLayout();
        repaint();
    }

    void ObsidianEnvelopeVisualizer::mouseMove(const juce::MouseEvent& event)
    {
        if (dragging_)
            return;

        hoveredHandle_ = handleAtPoint(event.position);
        updateHoverCursor(hoveredHandle_);
        repaint();
    }

    void ObsidianEnvelopeVisualizer::mouseDown(const juce::MouseEvent& event)
    {
        activeHandle_ = handleAtPoint(event.position);
        if (activeHandle_ == HandleKind::None)
            return;

        dragging_ = true;
        applyDrag(activeHandle_, event.position);
        repaint();
    }

    void ObsidianEnvelopeVisualizer::mouseDrag(const juce::MouseEvent& event)
    {
        if (activeHandle_ == HandleKind::None)
            return;

        applyDrag(activeHandle_, event.position);
        repaint();
    }

    void ObsidianEnvelopeVisualizer::mouseUp(const juce::MouseEvent&)
    {
        dragging_ = false;
        activeHandle_ = HandleKind::None;
        endActiveGestures();
        hoveredHandle_ = HandleKind::None;
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }

    void ObsidianEnvelopeVisualizer::mouseExit(const juce::MouseEvent&)
    {
        if (dragging_)
            return;

        hoveredHandle_ = HandleKind::None;
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }

    void ObsidianEnvelopeVisualizer::paintGrid(juce::Graphics& g, juce::Rectangle<float> bounds) const
    {
        g.setColour(palette::kBorder.withAlpha(0.15f));

        for (int i = 1; i < 4; ++i)
        {
            const float y = bounds.getY() + bounds.getHeight() * static_cast<float>(i) / 4.0f;
            g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight());
        }

        for (int i = 1; i < 6; ++i)
        {
            const float x = bounds.getX() + bounds.getWidth() * static_cast<float>(i) / 6.0f;
            g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
        }
    }

    void ObsidianEnvelopeVisualizer::paintHandles(juce::Graphics& g) const
    {
        const HandleKind handles[] = {HandleKind::DelayEnd, HandleKind::AttackPeak, HandleKind::HoldEnd,
                                      HandleKind::DecaySustain, HandleKind::ReleaseEnd};

        for (const auto kind : handles)
        {
            const auto pos = handlePosition(kind);
            const bool active = kind == activeHandle_;
            const bool hovered = kind == hoveredHandle_;

            g.setColour(palette::kPanel);
            g.fillEllipse(pos.x - kHandleRadius, pos.y - kHandleRadius, kHandleRadius * 2.0f, kHandleRadius * 2.0f);

            g.setColour(active ? palette::kTextPrimary : (hovered ? palette::kAccent : palette::kBorderBright));
            g.drawEllipse(pos.x - kHandleRadius, pos.y - kHandleRadius, kHandleRadius * 2.0f, kHandleRadius * 2.0f,
                          active ? 2.0f : 1.4f);

            if (active || hovered)
            {
                g.setColour(palette::kAccent.withAlpha(0.25f));
                g.drawEllipse(pos.x - kHandleRadius - 2.0f, pos.y - kHandleRadius - 2.0f, (kHandleRadius + 2.0f) * 2.0f,
                              (kHandleRadius + 2.0f) * 2.0f, 1.0f);
            }
        }
    }

    void ObsidianEnvelopeVisualizer::paintStageLabels(juce::Graphics& g) const
    {
        struct StageLabel
        {
            const char* text;
            float startTime;
            float endTime;
        };

        const StageLabel stages[] = {
            {"D", 0.0f, layout_.delayDisplay},
            {"A", layout_.delayDisplay, layout_.delayDisplay + layout_.attackDisplay},
            {"H", layout_.delayDisplay + layout_.attackDisplay,
             layout_.delayDisplay + layout_.attackDisplay + layout_.holdDisplay},
            {"D", layout_.delayDisplay + layout_.attackDisplay + layout_.holdDisplay,
             layout_.delayDisplay + layout_.attackDisplay + layout_.holdDisplay + layout_.decayDisplay},
            {"S", layout_.delayDisplay + layout_.attackDisplay + layout_.holdDisplay + layout_.decayDisplay,
             layout_.delayDisplay + layout_.attackDisplay + layout_.holdDisplay + layout_.decayDisplay
                 + layout_.sustainDisplay},
            {"R", layout_.delayDisplay + layout_.attackDisplay + layout_.holdDisplay + layout_.decayDisplay
                      + layout_.sustainDisplay,
             layout_.totalDisplaySeconds},
        };

        g.setFont(fonts::label(8.5f));
        g.setColour(palette::kTextDim);

        const float markerY = layout_.bounds.getBottom() + 2.0f;
        for (const auto& stage : stages)
        {
            if (stage.endTime <= stage.startTime)
                continue;

            const float mid = (stage.startTime + stage.endTime) * 0.5f;
            const float x = layout_.xFromDisplayTime(mid);
            g.drawText(stage.text, juce::Rectangle<float>(x - 8.0f, markerY, 16.0f, 12.0f), juce::Justification::centred);
        }
    }

    void ObsidianEnvelopeVisualizer::paint(juce::Graphics& g)
    {
        auto bounds = graphBounds();
        if (bounds.isEmpty())
            return;

        rebuildLayout();

        draw::fillRecessedRoundedRect(g, bounds, 6.0f);
        paintGrid(g, bounds);

        juce::Path outline;
        juce::Path fillPath;
        float sustainEndX = bounds.getX();
        wireframe::buildEnvelopePath(params_, bounds, outline, fillPath, sustainEndX);

        g.setColour(palette::kAccent.withAlpha(0.10f));
        g.fillPath(fillPath);
        draw::strokeGlowPath(g, outline, 1.0f, 2.0f, true);

        g.setColour(palette::kBorderBright.withAlpha(0.55f));
        g.drawHorizontalLine(static_cast<int>(layout_.baselineY), bounds.getX(), bounds.getRight());

        paintStageLabels(g);
        paintHandles(g);

        g.setColour(palette::kAccentWarm.withAlpha(0.85f));
        g.drawVerticalLine(static_cast<int>(sustainEndX), bounds.getY(), bounds.getBottom());
        g.setFont(fonts::label(8.0f));
        g.setColour(palette::kTextDim);
        g.drawText("note off", juce::Rectangle<float>(sustainEndX + 3.0f, bounds.getY(), 48.0f, 12.0f),
                   juce::Justification::centredLeft);
    }

} // namespace pw8::plugin::ui
