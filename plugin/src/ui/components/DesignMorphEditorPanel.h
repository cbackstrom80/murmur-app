#pragma once

#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "MorphTimelineStrip.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Figma `murmur-mi-ui-design-morph-editor` (`89:953`) — per-path override table, color picker, 16-keyframe cap.
    class DesignMorphEditorPanel : public juce::Component
    {
    public:
        explicit DesignMorphEditorPanel(PatchworkEightProcessor& processor);
        ~DesignMorphEditorPanel() override;

        std::function<void()> onClosed;
        std::function<void()> onOpenModMatrix;

        void showOverlay();
        void dismiss();
        void setEmbeddedInDesignMode(bool embedded);
        void refreshFromPatch();

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;
        bool keyPressed(const juce::KeyPress& key) override;

        friend class KeyframeListModel;
        friend class OverrideListModel;

    private:
        class KeyframeListModel;
        class OverrideListModel;

        void rebuildKeyframeList();
        void selectKeyframe(std::size_t index);
        void applySettingsFromControls();
        void styleCurveButton(juce::TextButton& btn, const juce::String& label, bool active);
        void cycleSelectedKeyframeColor();
        juce::Rectangle<int> colorSwatchBounds(std::size_t swatchIndex) const;

        PatchworkEightProcessor& processor_;
        juce::TextButton backButton_{"← ENGINE"};
        juce::Label titleLabel_;
        juce::Label subtitleLabel_;
        juce::Label keyframeHeaderLabel_;
        MorphTimelineStrip timeline_;
        std::unique_ptr<GlowKnob> morphPositionKnob_;
        juce::ListBox keyframeList_;
        std::unique_ptr<KeyframeListModel> keyframeModel_;
        juce::ListBox overrideList_;
        std::unique_ptr<OverrideListModel> overrideModel_;
        juce::Label overrideHeaderLabel_;
        juce::Label colorLabel_;
        juce::TextButton addKeyframeButton_{"+ ADD"};
        juce::TextButton recaptureButton_{"UPDATE"};
        juce::TextButton deleteKeyframeButton_{"DELETE SELECTED"};
        juce::ToggleButton wrapToggle_{"WRAP"};
        juce::ToggleButton disseminationToggle_{"DISSEMINATE @ NOTE-ON"};
        std::array<juce::TextButton, 7> curveButtons_{};
        std::array<juce::TextButton, 6> autoplayButtons_{};
        juce::Label curveLabel_;
        juce::Label autoplayLabel_;
        juce::TextButton openModMatrixButton_{"OPEN MOD MATRIX"};
        juce::Label footerHint_;
        std::size_t selectedKeyframe_ = 0;
        bool embeddedInDesignMode_ = false;
    };

} // namespace pw8::plugin::ui
