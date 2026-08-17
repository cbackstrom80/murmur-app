#include "QuasarSpatialWireframeView.h"

#include <cmath>

#include "ui/theme/ObsidianDraw.h"
#include "ui/theme/ObsidianFonts.h"
#include "ui/theme/ObsidianPalette.h"

namespace pw8::quasar::ui
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
    } // namespace

    QuasarSpatialWireframeView::QuasarSpatialWireframeView(juce::AudioProcessorValueTreeState& apvts) : apvts_(apvts)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void QuasarSpatialWireframeView::bindParameters(const juce::String& qsr1AngleId, const juce::String& qsr1DistanceId,
                                                    const juce::String& qsr1HeightId, const juce::String& qsr2AngleId,
                                                    const juce::String& qsr2DistanceId, const juce::String& qsr2HeightId)
    {
        qsr1AngleId_ = qsr1AngleId;
        qsr1DistanceId_ = qsr1DistanceId;
        qsr1HeightId_ = qsr1HeightId;
        qsr2AngleId_ = qsr2AngleId;
        qsr2DistanceId_ = qsr2DistanceId;
        qsr2HeightId_ = qsr2HeightId;
        repaint();
    }

    QuasarSpatialWireframeView::ViewFrame QuasarSpatialWireframeView::computeViewFrame() const
    {
        ViewFrame frame;
        frame.bounds = getLocalBounds().toFloat().reduced(4.0f);
        frame.headCentre = {frame.bounds.getCentreX(), frame.bounds.getY() + frame.bounds.getHeight() * 0.58f};
        frame.scale = juce::jmin(frame.bounds.getWidth(), frame.bounds.getHeight()) * 0.92f;
        return frame;
    }

    QuasarSpatialWireframeView::SourcePose QuasarSpatialWireframeView::readPose(const juce::String& angleId,
                                                                               const juce::String& distanceId,
                                                                               const juce::String& heightId) const
    {
        SourcePose pose;
        pose.angleDeg = readParam(apvts_, angleId, 0.0f);
        pose.distance = juce::jlimit(0.0f, 1.0f, readParam(apvts_, distanceId, 0.35f));
        pose.height = juce::jlimit(-1.0f, 1.0f, readParam(apvts_, heightId, 0.0f));
        return pose;
    }

    juce::Point<float> QuasarSpatialWireframeView::projectPose(const ViewFrame& frame,
                                                               const SourcePose& pose) const
    {
        const float az = pose.angleDeg * juce::MathConstants<float>::pi / 180.0f;
        const float radius = juce::jmap(pose.distance, 0.0f, 1.0f, 0.12f, 0.42f);
        const float x = radius * std::sin(az);
        const float y = -pose.height * 0.28f;
        const float z = -radius * std::cos(az);

        return {frame.headCentre.x + (x + z * 0.38f) * frame.scale,
                frame.headCentre.y + (y + z * 0.62f) * frame.scale};
    }

    QuasarSpatialWireframeView::SourcePose QuasarSpatialWireframeView::unprojectPoint(const ViewFrame& frame,
                                                                                     juce::Point<float> point) const
    {
        SourcePose pose;
        if (frame.scale <= 1.0e-3f)
            return pose;

        const float dx = (point.x - frame.headCentre.x) / frame.scale;
        const float dy = (point.y - frame.headCentre.y) / frame.scale;

        const float z = juce::jlimit(-0.55f, 0.05f, dy * 0.55f);
        const float x = dx - z * 0.38f;
        const float radius = juce::jlimit(0.12f, 0.42f, std::sqrt(x * x + z * z));

        pose.angleDeg = wrapDegrees360(std::atan2(x, -z) * 180.0f / juce::MathConstants<float>::pi);
        pose.distance = juce::jmap(radius, 0.12f, 0.42f, 0.0f, 1.0f);
        pose.height = juce::jlimit(-1.0f, 1.0f, -dy / 0.28f);
        return pose;
    }

    std::optional<QuasarSpatialWireframeView::DragTarget> QuasarSpatialWireframeView::hitTestMarker(
        juce::Point<float> point, const ViewFrame& frame) const
    {
        constexpr float kHitRadius = 18.0f;
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

    void QuasarSpatialWireframeView::setParamValue(const juce::String& id, float value)
    {
        if (auto* param = apvts_.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    }

    void QuasarSpatialWireframeView::writePose(DragTarget target, const SourcePose& pose)
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

    void QuasarSpatialWireframeView::mouseDown(const juce::MouseEvent& event)
    {
        const auto frame = computeViewFrame();
        if (const auto hit = hitTestMarker(event.position, frame))
            dragTarget_ = *hit;
        else
            dragTarget_ = DragTarget::None;
    }

    void QuasarSpatialWireframeView::mouseDrag(const juce::MouseEvent& event)
    {
        if (dragTarget_ == DragTarget::None)
            return;

        const auto frame = computeViewFrame();
        writePose(dragTarget_, unprojectPoint(frame, event.position));
        repaint();
    }

    void QuasarSpatialWireframeView::mouseUp(const juce::MouseEvent& /*event*/)
    {
        dragTarget_ = DragTarget::None;
    }

    void QuasarSpatialWireframeView::paintHeadWireframe(juce::Graphics& g, juce::Point<float> headCentre) const
    {
        juce::Path head;
        head.addEllipse(headCentre.x - 18.0f, headCentre.y - 24.0f, 36.0f, 44.0f);
        juce::Path band;
        band.addEllipse(headCentre.x - 30.0f, headCentre.y - 8.0f, 60.0f, 18.0f);
        draw::strokeGlowPath(g, head, 0.85f, 1.4f, true);
        g.setColour(palette::kBorderBright.withAlpha(0.45f));
        g.strokePath(band, juce::PathStrokeType(1.0f));
    }

    void QuasarSpatialWireframeView::paintFloorGrid(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                    juce::Point<float> headCentre) const
    {
        const float gridTop = headCentre.y + 8.0f;
        const float gridBottom = bounds.getBottom() - 8.0f;
        const float gridLeft = bounds.getX() + 16.0f;
        const float gridRight = bounds.getRight() - 16.0f;
        const float midX = (gridLeft + gridRight) * 0.5f;

        juce::Path grid;
        for (int i = 0; i <= 4; ++i)
        {
            const float t = static_cast<float>(i) / 4.0f;
            const float y = juce::jmap(t, 0.0f, 1.0f, gridTop, gridBottom);
            const float inset = t * 26.0f;
            grid.startNewSubPath(gridLeft + inset, y);
            grid.lineTo(gridRight - inset, y);
        }
        for (int i = 0; i <= 5; ++i)
        {
            const float t = static_cast<float>(i) / 5.0f;
            const float x = juce::jmap(t, 0.0f, 1.0f, gridLeft, gridRight);
            grid.startNewSubPath(x, gridTop);
            grid.lineTo(midX + (x - midX) * 0.35f, gridBottom);
        }

        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.strokePath(grid, juce::PathStrokeType(0.9f));
    }

    void QuasarSpatialWireframeView::paintSource(juce::Graphics& g, juce::Point<float> headCentre,
                                                 juce::Point<float> sourcePoint, const juce::String& label,
                                                 juce::Colour colour, bool isLeft, bool isDragging) const
    {
        juce::Path tether;
        tether.startNewSubPath(headCentre.x, headCentre.y - 6.0f);
        tether.lineTo(sourcePoint);
        g.setColour(colour.withAlpha(isDragging ? 0.55f : 0.35f));
        g.strokePath(tether, juce::PathStrokeType(isDragging ? 1.8f : 1.2f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

        draw::fillGlowDot(g, sourcePoint, isDragging ? 9.0f : (isLeft ? 7.5f : 7.0f), colour, isDragging ? 1.2f : 1.0f,
                          5);

        auto labelBounds = juce::Rectangle<float>(sourcePoint.x - 34.0f, sourcePoint.y - 28.0f, 68.0f, 14.0f);
        draw::drawLegibleText(g, label, labelBounds, juce::Justification::centred, colour, fonts::label(9.5f), true);
    }

    void QuasarSpatialWireframeView::paint(juce::Graphics& g)
    {
        const auto frame = computeViewFrame();
        draw::fillRecessedRoundedRect(g, frame.bounds, 10.0f);

        const auto qsr1 = readPose(qsr1AngleId_, qsr1DistanceId_, qsr1HeightId_);
        const auto qsr2 = readPose(qsr2AngleId_, qsr2DistanceId_, qsr2HeightId_);
        const juce::Point<float> qsr1Point = projectPose(frame, qsr1);
        const juce::Point<float> qsr2Point = projectPose(frame, qsr2);

        paintFloorGrid(g, frame.bounds, frame.headCentre);
        paintHeadWireframe(g, frame.headCentre);
        paintSource(g, frame.headCentre, qsr1Point, "L FEED", palette::kAccent, true, dragTarget_ == DragTarget::Qsr1);
        paintSource(g, frame.headCentre, qsr2Point, "R FEED", palette::kMurmurViolet, false,
                    dragTarget_ == DragTarget::Qsr2);

        auto titleBounds = frame.bounds.withTrimmedTop(0.0f);
        titleBounds.setHeight(18.0f);
        titleBounds.reduce(8.0f, 0.0f);
        draw::drawLegibleText(g, "HEADPHONE SPACE · drag markers", titleBounds, juce::Justification::centredLeft,
                              palette::kTextDim, fonts::label(10.0f), false);
    }

} // namespace pw8::quasar::ui
