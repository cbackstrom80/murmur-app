#pragma once

#include <functional>
#include <optional>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

// Headphone-space wireframe: projects QSR1 (L feed) and QSR2 (R feed) locations in
// pseudo-3D. Markers are draggable — writes angle / distance / height APVTS params.
namespace pw8::quasar::ui
{
    class QuasarSpatialWireframeView : public juce::Component
    {
    public:
        explicit QuasarSpatialWireframeView(juce::AudioProcessorValueTreeState& apvts);

        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;

        void bindParameters(const juce::String& qsr1AngleId, const juce::String& qsr1DistanceId,
                            const juce::String& qsr1HeightId, const juce::String& qsr2AngleId,
                            const juce::String& qsr2DistanceId, const juce::String& qsr2HeightId);

    private:
        enum class DragTarget
        {
            None,
            Qsr1,
            Qsr2,
        };

        struct SourcePose
        {
            float angleDeg = 0.0f;
            float distance = 0.35f;
            float height = 0.0f;
        };

        struct ViewFrame
        {
            juce::Rectangle<float> bounds;
            juce::Point<float> headCentre;
            float scale = 1.0f;
        };

        [[nodiscard]] ViewFrame computeViewFrame() const;
        [[nodiscard]] SourcePose readPose(const juce::String& angleId, const juce::String& distanceId,
                                          const juce::String& heightId) const;
        [[nodiscard]] juce::Point<float> projectPose(const ViewFrame& frame, const SourcePose& pose) const;
        [[nodiscard]] SourcePose unprojectPoint(const ViewFrame& frame, juce::Point<float> point) const;
        [[nodiscard]] std::optional<DragTarget> hitTestMarker(juce::Point<float> point, const ViewFrame& frame) const;

        void writePose(DragTarget target, const SourcePose& pose);
        void setParamValue(const juce::String& id, float value);

        void paintHeadWireframe(juce::Graphics& g, juce::Point<float> headCentre) const;
        void paintFloorGrid(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Point<float> headCentre) const;
        void paintSource(juce::Graphics& g, juce::Point<float> headCentre, juce::Point<float> sourcePoint,
                         const juce::String& label, juce::Colour colour, bool isLeft, bool isDragging) const;

        juce::AudioProcessorValueTreeState& apvts_;
        juce::String qsr1AngleId_{"qsr1Angle"};
        juce::String qsr1DistanceId_{"qsr1Distance"};
        juce::String qsr1HeightId_{"qsr1Height"};
        juce::String qsr2AngleId_{"qsr2Angle"};
        juce::String qsr2DistanceId_{"qsr2Distance"};
        juce::String qsr2HeightId_{"qsr2Height"};

        DragTarget dragTarget_ = DragTarget::None;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuasarSpatialWireframeView)
    };

} // namespace pw8::quasar::ui
