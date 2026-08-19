#include "QuasarBinauralFieldView.h"

#include <cmath>

#include "../../theme/ObsidianDraw.h"
#include "../../visualizer/VisualizerGpu.h"
#include "../../theme/ObsidianFonts.h"
#include "../../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] float readParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id,
                                      float fallback) noexcept
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        }

        [[nodiscard]] float wrapDegrees360(float degrees) noexcept
        {
            float wrapped = std::fmod(degrees, 360.0f);
            if (wrapped < 0.0f)
                wrapped += 360.0f;
            return wrapped;
        }

        [[nodiscard]] juce::String paramPrefixFromAngleId(const juce::String& angleId)
        {
            return angleId.upToLastOccurrenceOf("Qsr1AngleDeg", false, true);
        }
    } // namespace

    QuasarBinauralFieldView::QuasarBinauralFieldView(juce::AudioProcessorValueTreeState& apvts) : apvts_(apvts)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void QuasarBinauralFieldView::attachVisualizerBus(murmur8::AudioVisualizerBus& bus)
    {
        visualizerBus_ = &bus;
        if (!murmur8::visualizerGpuEnabled())
            return;

        glPlot_ = std::make_unique<murmur8::MurmurVisualizerComponent>(bus);
        glPlot_->setMode(murmur8::MurmurVisualizerComponent::Mode::QuasarBinaural);
        addAndMakeVisible(*glPlot_);
        syncGlFieldParams();
        resized();
    }

    void QuasarBinauralFieldView::bindParameters(const juce::String& qsr1AngleId, const juce::String& qsr1DistanceId,
                                                 const juce::String& qsr1HeightId, const juce::String& qsr2AngleId,
                                                 const juce::String& qsr2DistanceId, const juce::String& qsr2HeightId)
    {
        qsr1AngleId_ = qsr1AngleId;
        qsr1DistanceId_ = qsr1DistanceId;
        qsr1HeightId_ = qsr1HeightId;
        qsr2AngleId_ = qsr2AngleId;
        qsr2DistanceId_ = qsr2DistanceId;
        qsr2HeightId_ = qsr2HeightId;
        qsr1Trail_.fill(readParam(apvts_, qsr1AngleId_, 30.0f));
        qsr2Trail_.fill(readParam(apvts_, qsr2AngleId_, 330.0f));
        syncGlFieldParams();
        repaint();
    }

    void QuasarBinauralFieldView::syncGlFieldParams()
    {
        if (glPlot_ == nullptr)
            return;

        const juce::String prefix = paramPrefixFromAngleId(qsr1AngleId_);
        murmur8::QuasarFieldParams params;
        params.qsr1AngleDeg = readParam(apvts_, qsr1AngleId_, 30.0f);
        params.qsr1Distance = readParam(apvts_, qsr1DistanceId_, 0.35f);
        params.qsr1Height = readParam(apvts_, qsr1HeightId_, 0.0f);
        params.qsr1Level = readParam(apvts_, prefix + "Qsr1Level", 1.0f);
        params.qsr2AngleDeg = readParam(apvts_, qsr2AngleId_, 330.0f);
        params.qsr2Distance = readParam(apvts_, qsr2DistanceId_, 0.35f);
        params.qsr2Height = readParam(apvts_, qsr2HeightId_, 0.0f);
        params.qsr2Level = readParam(apvts_, prefix + "Qsr2Level", 1.0f);
        params.crossfeed = readParam(apvts_, prefix + "QuasarCrossfeed", 0.0f);
        params.width = readParam(apvts_, prefix + "CntrLevel", 0.5f);
        params.mix = readParam(apvts_, prefix + "Mix", 1.0f);
        glPlot_->setQuasarFieldParams(params);
    }

    void QuasarBinauralFieldView::resized()
    {
        if (glPlot_ != nullptr)
            glPlot_->setBounds(getLocalBounds());
    }

    void QuasarBinauralFieldView::pushTrailSample()
    {
        qsr1Trail_[trailWriteIndex_ % kTrailLength] = readParam(apvts_, qsr1AngleId_, 30.0f);
        qsr2Trail_[trailWriteIndex_ % kTrailLength] = readParam(apvts_, qsr2AngleId_, 330.0f);
        ++trailWriteIndex_;
        syncGlFieldParams();
    }

    QuasarBinauralFieldView::ViewFrame QuasarBinauralFieldView::computeViewFrame() const
    {
        ViewFrame frame;
        frame.bounds = getLocalBounds().toFloat().reduced(6.0f);
        frame.headCentre = frame.bounds.getCentre();
        frame.ringRadius = juce::jmin(frame.bounds.getWidth(), frame.bounds.getHeight()) * 0.36f;
        return frame;
    }

    QuasarBinauralFieldView::SourcePose QuasarBinauralFieldView::readPose(const juce::String& angleId,
                                                                          const juce::String& distanceId,
                                                                          const juce::String& heightId) const
    {
        SourcePose pose;
        pose.angleDeg = readParam(apvts_, angleId, 30.0f);
        pose.distance = juce::jlimit(0.0f, 1.0f, readParam(apvts_, distanceId, 0.35f));
        pose.height = juce::jlimit(-1.0f, 1.0f, readParam(apvts_, heightId, 0.0f));
        return pose;
    }

    juce::Point<float> QuasarBinauralFieldView::projectPose(const ViewFrame& frame, const SourcePose& pose) const
    {
        const float az = juce::degreesToRadians(pose.angleDeg - 90.0f);
        const float radiusNorm = juce::jmap(pose.distance, 0.0f, 1.0f, 0.22f, 1.0f);
        const float radius = frame.ringRadius * radiusNorm;
        const float elevOffset = -pose.height * frame.ringRadius * 0.18f;
        return {frame.headCentre.x + std::cos(az) * radius,
                frame.headCentre.y + std::sin(az) * radius + elevOffset};
    }

    QuasarBinauralFieldView::SourcePose QuasarBinauralFieldView::unprojectPoint(const ViewFrame& frame,
                                                                                juce::Point<float> point) const
    {
        SourcePose pose;
        if (frame.ringRadius <= 1.0e-3f)
            return pose;

        const float dx = point.x - frame.headCentre.x;
        const float dy = point.y - frame.headCentre.y;
        pose.angleDeg = wrapDegrees360(std::atan2(dy, dx) * 180.0f / juce::MathConstants<float>::pi + 90.0f);
        const float distPx = std::sqrt(dx * dx + dy * dy);
        pose.distance = juce::jlimit(0.0f, 1.0f, juce::jmap(distPx, frame.ringRadius * 0.22f, frame.ringRadius, 0.0f, 1.0f));
        pose.height = juce::jlimit(-1.0f, 1.0f, -dy / (frame.ringRadius * 0.55f));
        return pose;
    }

    std::optional<QuasarBinauralFieldView::DragTarget> QuasarBinauralFieldView::hitTestMarker(
        juce::Point<float> point, const ViewFrame& frame) const
    {
        constexpr float kHitRadius = 16.0f;
        const auto qsr1Point = projectPose(frame, readPose(qsr1AngleId_, qsr1DistanceId_, qsr1HeightId_));
        const auto qsr2Point = projectPose(frame, readPose(qsr2AngleId_, qsr2DistanceId_, qsr2HeightId_));
        const float d1 = point.getDistanceFrom(qsr1Point);
        const float d2 = point.getDistanceFrom(qsr2Point);
        if (d1 <= kHitRadius && d1 <= d2)
            return DragTarget::Qsr1;
        if (d2 <= kHitRadius)
            return DragTarget::Qsr2;
        return std::nullopt;
    }

    void QuasarBinauralFieldView::setParamValue(const juce::String& id, float value)
    {
        if (auto* param = apvts_.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    }

    void QuasarBinauralFieldView::writePose(DragTarget target, const SourcePose& pose)
    {
        switch (target)
        {
            case DragTarget::Qsr1:
                setParamValue(qsr1AngleId_, pose.angleDeg);
                setParamValue(qsr1DistanceId_, pose.distance);
                setParamValue(qsr1HeightId_, pose.height);
                break;
            case DragTarget::Qsr2:
                setParamValue(qsr2AngleId_, pose.angleDeg);
                setParamValue(qsr2DistanceId_, pose.distance);
                setParamValue(qsr2HeightId_, pose.height);
                break;
            case DragTarget::None:
                break;
        }
    }

    void QuasarBinauralFieldView::paintMarkerOverlay(juce::Graphics& g, const ViewFrame& frame) const
    {
        const auto qsr1 = readPose(qsr1AngleId_, qsr1DistanceId_, qsr1HeightId_);
        const auto qsr2 = readPose(qsr2AngleId_, qsr2DistanceId_, qsr2HeightId_);
        const auto p1 = projectPose(frame, qsr1);
        const auto p2 = projectPose(frame, qsr2);

        if (dragTarget_ == DragTarget::Qsr1)
            draw::fillGlowDot(g, p1, 11.0f, juce::Colour(0xff00c8ff), 1.2f, 6);
        if (dragTarget_ == DragTarget::Qsr2)
            draw::fillGlowDot(g, p2, 11.0f, juce::Colour(0xffe040fb), 1.2f, 6);

        draw::drawLegibleText(g, "QSR1", juce::Rectangle<float>(p1.x - 30.0f, p1.y - 24.0f, 60.0f, 14.0f),
                              juce::Justification::centred, juce::Colour(0xff00c8ff), fonts::label(9.0f), true);
        draw::drawLegibleText(g, "QSR2", juce::Rectangle<float>(p2.x - 30.0f, p2.y - 24.0f, 60.0f, 14.0f),
                              juce::Justification::centred, juce::Colour(0xffe040fb), fonts::label(9.0f), true);
    }

    void QuasarBinauralFieldView::mouseDown(const juce::MouseEvent& event)
    {
        const auto frame = computeViewFrame();
        if (const auto hit = hitTestMarker(event.position, frame))
            dragTarget_ = *hit;
        else
            dragTarget_ = DragTarget::None;
    }

    void QuasarBinauralFieldView::mouseDrag(const juce::MouseEvent& event)
    {
        if (dragTarget_ == DragTarget::None)
            return;
        const auto frame = computeViewFrame();
        writePose(dragTarget_, unprojectPoint(frame, event.position));
        pushTrailSample();
        repaint();
    }

    void QuasarBinauralFieldView::mouseUp(const juce::MouseEvent& /*event*/)
    {
        dragTarget_ = DragTarget::None;
        repaint();
    }

    void QuasarBinauralFieldView::paint(juce::Graphics& g)
    {
        const auto frame = computeViewFrame();

        if (glPlot_ == nullptr || !murmur8::visualizerGpuEnabled())
        {
            draw::fillRecessedRoundedRect(g, frame.bounds, 10.0f);
            g.setColour(palette::kBorder.withAlpha(0.35f));
            g.drawEllipse(frame.headCentre.x - frame.ringRadius, frame.headCentre.y - frame.ringRadius,
                          frame.ringRadius * 2.0f, frame.ringRadius * 2.0f, 1.2f);
            g.setColour(palette::kAccent.withAlpha(0.25f));
            g.fillEllipse(frame.headCentre.x - 6.0f, frame.headCentre.y - 6.0f, 12.0f, 12.0f);

            auto titleBounds = frame.bounds.withTrimmedTop(0.0f);
            titleBounds.setHeight(18.0f);
            titleBounds.reduce(10.0f, 0.0f);
            draw::drawLegibleText(g, "BINAURAL FIELD · drag QSR markers", titleBounds, juce::Justification::centredLeft,
                                  palette::kTextDim, fonts::label(10.0f), false);
            paintMarkerOverlay(g, frame);
            return;
        }

        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 10.0f, 1.0f);

        auto titleBounds = frame.bounds.withTrimmedTop(0.0f);
        titleBounds.setHeight(18.0f);
        titleBounds.reduce(10.0f, 0.0f);
        draw::drawLegibleText(g, "BINAURAL FIELD · drag QSR markers", titleBounds, juce::Justification::centredLeft,
                              palette::kTextDim, fonts::label(10.0f), false);

        paintMarkerOverlay(g, frame);
    }

} // namespace pw8::plugin::ui
