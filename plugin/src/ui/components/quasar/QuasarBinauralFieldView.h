#pragma once

#include <array>
#include <memory>
#include <optional>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../../visualizer/MurmurVisualizerComponent.h"

namespace pw8::plugin::ui
{
    /// Figma `102:4` binaural hero — polar azimuth ring + draggable QSR1/QSR2 markers.
    class QuasarBinauralFieldView : public juce::Component
    {
    public:
        static constexpr std::size_t kTrailLength = 32;

        explicit QuasarBinauralFieldView(juce::AudioProcessorValueTreeState& apvts);

        void attachVisualizerBus(murmur8::AudioVisualizerBus& bus);

        void bindParameters(const juce::String& qsr1AngleId, const juce::String& qsr1DistanceId,
                            const juce::String& qsr1HeightId, const juce::String& qsr2AngleId,
                            const juce::String& qsr2DistanceId, const juce::String& qsr2HeightId);

        void pushTrailSample();

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;

    private:
        struct ViewFrame
        {
            juce::Rectangle<float> bounds;
            juce::Point<float> headCentre;
            float ringRadius = 0.0f;
        };

        struct SourcePose
        {
            float angleDeg = 0.0f;
            float distance = 0.35f;
            float height = 0.0f;
        };

        enum class DragTarget : std::uint8_t
        {
            None,
            Qsr1,
            Qsr2,
        };

        [[nodiscard]] ViewFrame computeViewFrame() const;
        [[nodiscard]] SourcePose readPose(const juce::String& angleId, const juce::String& distanceId,
                                          const juce::String& heightId) const;
        [[nodiscard]] juce::Point<float> projectPose(const ViewFrame& frame, const SourcePose& pose) const;
        [[nodiscard]] SourcePose unprojectPoint(const ViewFrame& frame, juce::Point<float> point) const;
        [[nodiscard]] std::optional<DragTarget> hitTestMarker(juce::Point<float> point, const ViewFrame& frame) const;
        void setParamValue(const juce::String& id, float value);
        void writePose(DragTarget target, const SourcePose& pose);
        void syncGlFieldParams();
        void paintMarkerOverlay(juce::Graphics& g, const ViewFrame& frame) const;

        juce::AudioProcessorValueTreeState& apvts_;
        murmur8::AudioVisualizerBus* visualizerBus_ = nullptr;
        std::unique_ptr<murmur8::MurmurVisualizerComponent> glPlot_;
        juce::String qsr1AngleId_{"MasterFx0Qsr1AngleDeg"};
        juce::String qsr1DistanceId_{"MasterFx0Qsr1Distance"};
        juce::String qsr1HeightId_{"MasterFx0Qsr1Height"};
        juce::String qsr2AngleId_{"MasterFx0Qsr2AngleDeg"};
        juce::String qsr2DistanceId_{"MasterFx0Qsr2Distance"};
        juce::String qsr2HeightId_{"MasterFx0Qsr2Height"};
        DragTarget dragTarget_ = DragTarget::None;
        std::array<float, kTrailLength> qsr1Trail_{};
        std::array<float, kTrailLength> qsr2Trail_{};
        std::size_t trailWriteIndex_ = 0;
    };

} // namespace pw8::plugin::ui
