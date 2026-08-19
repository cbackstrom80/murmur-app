#include "DesignMorphEditorPanel.h"

#include "../PlayModeLayout.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "pw8/core/Types.hpp"
#include "pw8/modulation/MorphEasing.hpp"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        static constexpr const char* kPaletteColors[] = {"#4a90d9", "#9f80ff", "#e0a040", "#40e0c0",
                                                         "#e06080", "#80e040", "#6080e0", "#e08040"};
    } // namespace

    class DesignMorphEditorPanel::KeyframeListModel : public juce::ListBoxModel
    {
    public:
        explicit KeyframeListModel(DesignMorphEditorPanel& owner) : owner_(owner) {}

        int getNumRows() override
        {
            return static_cast<int>(owner_.processor_.getCurrentPatch().morphKoin.keyframes.size());
        }

        void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected) override
        {
            const auto& mk = owner_.processor_.getCurrentPatch().morphKoin;
            if (row < 0 || static_cast<std::size_t>(row) >= mk.keyframes.size())
                return;

            const auto& kf = mk.keyframes[static_cast<std::size_t>(row)];
            auto bounds = juce::Rectangle<int>(0, 0, width, height).reduced(0, 2).toFloat();

            g.setColour(selected ? palette::kAccent.withAlpha(0.14f) : palette::kPanelRaised.withAlpha(0.55f));
            g.fillRoundedRectangle(bounds, 4.0f);
            if (selected)
            {
                g.setColour(palette::kAccent.withAlpha(0.85f));
                g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
            }

            if (!kf.color.empty() && kf.color.front() == '#')
            {
                const auto hex = juce::String(kf.color.c_str()).substring(1);
                g.setColour(juce::Colour::fromString("ff" + hex));
                g.fillEllipse(bounds.getX() + 8.0f, bounds.getCentreY() - 3.0f, 6.0f, 6.0f);
            }

            g.setFont(fonts::label(8.0f));
            g.setColour(palette::kTextPrimary);
            g.drawText("KF" + juce::String(row + 1) + " " + juce::String(kf.name.c_str()),
                       bounds.reduced(18.0f, 0.0f).removeFromLeft(bounds.getWidth() * 0.72f),
                       juce::Justification::centredLeft, true);

            g.setColour(palette::kTextDim);
            g.setFont(fonts::label(7.0f));
            g.drawText(juce::String(kf.position, 2), bounds.reduced(8.0f, 0.0f), juce::Justification::centredRight,
                       true);
        }

        void listBoxItemClicked(int row, const juce::MouseEvent&) override
        {
            if (row >= 0)
                owner_.selectKeyframe(static_cast<std::size_t>(row));
        }

    private:
        DesignMorphEditorPanel& owner_;
    };

    class DesignMorphEditorPanel::OverrideListModel : public juce::ListBoxModel
    {
    public:
        explicit OverrideListModel(DesignMorphEditorPanel& owner) : owner_(owner) {}

        int getNumRows() override
        {
            const auto& mk = owner_.processor_.getCurrentPatch().morphKoin;
            if (owner_.selectedKeyframe_ >= mk.keyframes.size())
                return 0;
            return static_cast<int>(mk.keyframes[owner_.selectedKeyframe_].paramOverrides.size());
        }

        void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected) override
        {
            const auto& mk = owner_.processor_.getCurrentPatch().morphKoin;
            if (owner_.selectedKeyframe_ >= mk.keyframes.size())
                return;

            const auto& overrides = mk.keyframes[owner_.selectedKeyframe_].paramOverrides;
            if (row < 0 || static_cast<std::size_t>(row) >= overrides.size())
                return;

            auto it = overrides.begin();
            std::advance(it, row);
            const auto& path = it->first;
            const auto& ov = it->second;

            auto bounds = juce::Rectangle<int>(0, 0, width, height).reduced(2, 1).toFloat();
            g.setColour(selected ? palette::kFigmaTeal.withAlpha(0.12f) : palette::kPanelRaised.withAlpha(0.35f));
            g.fillRoundedRectangle(bounds, 3.0f);

            g.setFont(fonts::label(7.0f));
            g.setColour(palette::kTextPrimary);
            g.drawText(juce::String(path.c_str()), bounds.reduced(6.0f, 0.0f).removeFromLeft(bounds.getWidth() * 0.55f),
                       juce::Justification::centredLeft, true);

            const juce::String easing =
                ov.easing.empty() ? "GLOBAL" : juce::String(pw8::modulation::morphEasingLabel(
                                                     pw8::modulation::parseMorphEasing(ov.easing)));
            g.setColour(palette::kTextDim);
            g.drawText(juce::String(ov.value, 2) + " · " + easing, bounds.reduced(6.0f, 0.0f),
                       juce::Justification::centredRight, true);
        }

        void listBoxItemClicked(int row, const juce::MouseEvent&) override
        {
            const auto& mk = owner_.processor_.getCurrentPatch().morphKoin;
            if (owner_.selectedKeyframe_ >= mk.keyframes.size())
                return;

            const auto& overrides = mk.keyframes[owner_.selectedKeyframe_].paramOverrides;
            if (row < 0 || static_cast<std::size_t>(row) >= overrides.size())
                return;

            auto it = overrides.begin();
            std::advance(it, row);
            const std::string& path = it->first;
            const std::string current = it->second.easing.empty() ? "linear" : it->second.easing;
            const auto next = pw8::modulation::cycleMorphEasing(pw8::modulation::parseMorphEasing(current));
            const char* nextId = "linear";
            switch (next)
            {
                case pw8::modulation::MorphEasing::Smooth: nextId = "smooth"; break;
                case pw8::modulation::MorphEasing::Step: nextId = "step"; break;
                case pw8::modulation::MorphEasing::InQuartic: nextId = "inQuartic"; break;
                case pw8::modulation::MorphEasing::OutQuartic: nextId = "outQuartic"; break;
                case pw8::modulation::MorphEasing::InOutSine: nextId = "sine"; break;
                case pw8::modulation::MorphEasing::Bounce: nextId = "bounce"; break;
                case pw8::modulation::MorphEasing::Linear: break;
            }
            owner_.processor_.setMorphKeyframeParamEasing(owner_.selectedKeyframe_, path, nextId);
            owner_.refreshFromPatch();
        }

    private:
        DesignMorphEditorPanel& owner_;
    };

    DesignMorphEditorPanel::DesignMorphEditorPanel(PatchworkEightProcessor& processor)
        : processor_(processor), timeline_(processor),
          morphPositionKnob_(std::make_unique<GlowKnob>(processor.apvts, kMorphPositionId, "MORPH POS",
                                                        [](float v) { return juce::String(v, 2); }))
    {
        keyframeModel_ = std::make_unique<KeyframeListModel>(*this);
        overrideModel_ = std::make_unique<OverrideListModel>(*this);
        keyframeList_.setRowHeight(layout::kDesignMorphEditorKeyframeCardHeight);
        keyframeList_.setOutlineThickness(0);
        overrideList_.setRowHeight(layout::kDesignMorphEditorOverrideRowHeight);
        overrideList_.setOutlineThickness(0);

        timeline_.setShowMorphKnob(false);

        backButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        backButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        backButton_.onClick = [this] {
            if (onClosed)
                onClosed();
        };
        addAndMakeVisible(backButton_);

        titleLabel_.setText("MORPH KEYFRAME EDITOR", juce::dontSendNotification);
        titleLabel_.setFont(fonts::label(12.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(titleLabel_);

        subtitleLabel_.setText("MOTION / MORPH · UP TO 16 KEYFRAMES · PER-PATH EASING", juce::dontSendNotification);
        subtitleLabel_.setFont(fonts::label(7.0f));
        subtitleLabel_.setColour(juce::Label::textColourId, palette::kAccent);
        addAndMakeVisible(subtitleLabel_);

        keyframeHeaderLabel_.setFont(fonts::label(8.0f));
        keyframeHeaderLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(keyframeHeaderLabel_);

        overrideHeaderLabel_.setText("PER-PATH OVERRIDES (click easing to cycle)", juce::dontSendNotification);
        overrideHeaderLabel_.setFont(fonts::label(7.0f));
        overrideHeaderLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(overrideHeaderLabel_);

        colorLabel_.setText("KEYFRAME COLOR", juce::dontSendNotification);
        colorLabel_.setFont(fonts::label(7.0f));
        colorLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(colorLabel_);

        morphPositionKnob_->applyFigmaContext(figma::KnobContext::OperatorHero);
        addAndMakeVisible(*morphPositionKnob_);
        addAndMakeVisible(timeline_);
        timeline_.onKeyframeSelected = [this](std::size_t index) { selectKeyframe(index); };

        addKeyframeButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        addKeyframeButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        addKeyframeButton_.onClick = [this] {
            const auto& mk = processor_.getCurrentPatch().morphKoin;
            if (mk.keyframes.size() >= core::kMaxMorphKeyframes)
                return;
            float pos = 0.0f;
            if (auto* raw = processor_.apvts.getRawParameterValue(kMorphPositionId))
                pos = raw->load();
            if (processor_.addMorphKeyframeAt(pos))
                refreshFromPatch();
        };
        addAndMakeVisible(addKeyframeButton_);

        recaptureButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        recaptureButton_.setColour(juce::TextButton::textColourOffId, palette::kTextDim);
        recaptureButton_.onClick = [this] {
            if (processor_.recaptureMorphKeyframe(selectedKeyframe_))
                refreshFromPatch();
        };
        addAndMakeVisible(recaptureButton_);

        deleteKeyframeButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        deleteKeyframeButton_.setColour(juce::TextButton::textColourOffId, palette::kAccentWarm);
        deleteKeyframeButton_.onClick = [this] {
            if (processor_.removeMorphKeyframe(selectedKeyframe_))
                refreshFromPatch();
        };
        addAndMakeVisible(deleteKeyframeButton_);

        wrapToggle_.setColour(juce::ToggleButton::textColourId, palette::kTextDim);
        wrapToggle_.setColour(juce::ToggleButton::tickColourId, palette::kAccent);
        wrapToggle_.onClick = [this] { applySettingsFromControls(); };
        addAndMakeVisible(wrapToggle_);

        disseminationToggle_.setColour(juce::ToggleButton::textColourId, palette::kTextDim);
        disseminationToggle_.setColour(juce::ToggleButton::tickColourId, palette::kFigmaTeal);
        disseminationToggle_.onClick = [this] { applySettingsFromControls(); };
        addAndMakeVisible(disseminationToggle_);

        curveLabel_.setText("CURVE", juce::dontSendNotification);
        curveLabel_.setFont(fonts::label(7.0f));
        curveLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(curveLabel_);

        static constexpr const char* kCurves[] = {"linear", "smooth", "step", "sine", "bounce", "inQuartic",
                                                  "outQuartic"};
        static constexpr const char* kCurveLabels[] = {"LIN", "SMO", "STP", "SIN", "BNC", "IN4", "OUT4"};
        for (std::size_t i = 0; i < curveButtons_.size(); ++i)
        {
            styleCurveButton(curveButtons_[i], kCurveLabels[i], i == 0);
            const juce::String curveId = kCurves[i];
            curveButtons_[i].onClick = [this, curveId] {
                const auto& mk = processor_.getCurrentPatch().morphKoin;
                processor_.setMorphKoinSettings(curveId.toStdString(), mk.wrap, mk.autoplaySource,
                                                processor_.getCurrentPatch().voiceSettings.morphDissemination);
                refreshFromPatch();
            };
            addAndMakeVisible(curveButtons_[i]);
        }

        autoplayLabel_.setText("AUTOPLAY", juce::dontSendNotification);
        autoplayLabel_.setFont(fonts::label(7.0f));
        autoplayLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(autoplayLabel_);

        static constexpr const char* kAutoplayIds[] = {"none", "lfo1", "lfo2", "lfo3", "lfo4", "modWheel"};
        static constexpr const char* kAutoplayLabels[] = {"OFF", "LFO1", "LFO2", "LFO3", "LFO4", "MW"};
        for (std::size_t i = 0; i < autoplayButtons_.size(); ++i)
        {
            styleCurveButton(autoplayButtons_[i], kAutoplayLabels[i], i == 0);
            const juce::String sourceId = kAutoplayIds[i];
            autoplayButtons_[i].onClick = [this, sourceId] {
                const auto& mk = processor_.getCurrentPatch().morphKoin;
                processor_.setMorphKoinSettings(mk.curve, mk.wrap, sourceId.toStdString(),
                                                processor_.getCurrentPatch().voiceSettings.morphDissemination);
                refreshFromPatch();
            };
            addAndMakeVisible(autoplayButtons_[i]);
        }

        openModMatrixButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        openModMatrixButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        openModMatrixButton_.onClick = [this] {
            if (onOpenModMatrix)
                onOpenModMatrix();
        };
        addAndMakeVisible(openModMatrixButton_);

        footerHint_.setText("MorphPosition mod dest overrides manual scrub · FR.STEP fires on keyframe cross",
                            juce::dontSendNotification);
        footerHint_.setFont(fonts::label(9.0f));
        footerHint_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(footerHint_);

        addAndMakeVisible(keyframeList_);
        keyframeList_.setModel(keyframeModel_.get());
        addAndMakeVisible(overrideList_);
        overrideList_.setModel(overrideModel_.get());
    }

    DesignMorphEditorPanel::~DesignMorphEditorPanel()
    {
        keyframeList_.setModel(nullptr);
        overrideList_.setModel(nullptr);
    }

    void DesignMorphEditorPanel::showOverlay()
    {
        setVisible(true);
        refreshFromPatch();
    }

    void DesignMorphEditorPanel::dismiss() { setVisible(false); }

    void DesignMorphEditorPanel::setEmbeddedInDesignMode(bool embedded)
    {
        embeddedInDesignMode_ = embedded;
        backButton_.setButtonText(embedded ? "← ENGINE" : "← PLAY BOARD");
    }

    void DesignMorphEditorPanel::refreshFromPatch()
    {
        const auto& mk = processor_.getCurrentPatch().morphKoin;
        const auto& vs = processor_.getCurrentPatch().voiceSettings;

        keyframeHeaderLabel_.setText("KEYFRAMES (" + juce::String(static_cast<int>(mk.keyframes.size())) + "/"
                                         + juce::String(static_cast<int>(core::kMaxMorphKeyframes)) + ")",
                                     juce::dontSendNotification);
        addKeyframeButton_.setEnabled(mk.keyframes.size() < core::kMaxMorphKeyframes);

        wrapToggle_.setToggleState(mk.wrap, juce::dontSendNotification);
        disseminationToggle_.setToggleState(vs.morphDissemination, juce::dontSendNotification);

        static constexpr const char* kCurves[] = {"linear", "smooth", "step", "sine", "bounce", "inQuartic",
                                                  "outQuartic"};
        for (std::size_t i = 0; i < curveButtons_.size(); ++i)
            styleCurveButton(curveButtons_[i], curveButtons_[i].getButtonText(), mk.curve == kCurves[i]);

        static constexpr const char* kAutoplayIds[] = {"none", "lfo1", "lfo2", "lfo3", "lfo4", "modWheel"};
        for (std::size_t i = 0; i < autoplayButtons_.size(); ++i)
            styleCurveButton(autoplayButtons_[i], autoplayButtons_[i].getButtonText(),
                             mk.autoplaySource == kAutoplayIds[i]);

        if (selectedKeyframe_ >= mk.keyframes.size())
            selectedKeyframe_ = mk.keyframes.empty() ? 0 : mk.keyframes.size() - 1;

        keyframeList_.updateContent();
        keyframeList_.repaint();
        overrideList_.updateContent();
        overrideList_.repaint();
        timeline_.refresh();
        repaint();
    }

    void DesignMorphEditorPanel::rebuildKeyframeList() { keyframeList_.updateContent(); }

    void DesignMorphEditorPanel::selectKeyframe(std::size_t index)
    {
        selectedKeyframe_ = index;
        keyframeList_.selectRow(static_cast<int>(index));
        keyframeList_.repaint();
        overrideList_.updateContent();
        overrideList_.repaint();

        const auto& mk = processor_.getCurrentPatch().morphKoin;
        if (index < mk.keyframes.size())
        {
            if (auto* param = processor_.apvts.getParameter(kMorphPositionId))
                param->setValueNotifyingHost(param->convertTo0to1(mk.keyframes[index].position));
        }
    }

    void DesignMorphEditorPanel::applySettingsFromControls()
    {
        const auto& mk = processor_.getCurrentPatch().morphKoin;
        processor_.setMorphKoinSettings(mk.curve, wrapToggle_.getToggleState(), mk.autoplaySource,
                                        disseminationToggle_.getToggleState());
        refreshFromPatch();
    }

    void DesignMorphEditorPanel::styleCurveButton(juce::TextButton& btn, const juce::String& label, bool active)
    {
        btn.setButtonText(label);
        btn.setClickingTogglesState(false);
        btn.setColour(juce::TextButton::buttonColourId, active ? palette::kAccent.withAlpha(0.12f) : palette::kPanelRaised);
        btn.setColour(juce::TextButton::textColourOffId, active ? palette::kAccent : palette::kTextDim);
    }

    juce::Rectangle<int> DesignMorphEditorPanel::colorSwatchBounds(std::size_t swatchIndex) const
    {
        const int x = colorLabel_.getX() + static_cast<int>(swatchIndex)
                      * (layout::kDesignMorphEditorColorSwatchSize + 4);
        return {x, colorLabel_.getBottom() + 4, layout::kDesignMorphEditorColorSwatchSize,
                layout::kDesignMorphEditorColorSwatchSize};
    }

    void DesignMorphEditorPanel::cycleSelectedKeyframeColor()
    {
        static constexpr std::size_t kCount = sizeof(kPaletteColors) / sizeof(kPaletteColors[0]);
        const auto& mk = processor_.getCurrentPatch().morphKoin;
        if (selectedKeyframe_ >= mk.keyframes.size())
            return;

        const auto& current = mk.keyframes[selectedKeyframe_].color;
        std::size_t next = 0;
        for (; next < kCount; ++next)
        {
            if (current == kPaletteColors[next])
                break;
        }
        processor_.setMorphKeyframeColor(selectedKeyframe_, kPaletteColors[(next + 1) % kCount]);
        refreshFromPatch();
    }

    bool DesignMorphEditorPanel::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::escapeKey)
        {
            if (onClosed)
                onClosed();
            return true;
        }
        return false;
    }

    void DesignMorphEditorPanel::paint(juce::Graphics& g)
    {
        g.fillAll(palette::kBackgroundTop);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));

        for (std::size_t i = 0; i < sizeof(kPaletteColors) / sizeof(kPaletteColors[0]); ++i)
        {
            const auto swatch = colorSwatchBounds(i).toFloat();
            const auto hex = juce::String(kPaletteColors[i]).substring(1);
            g.setColour(juce::Colour::fromString("ff" + hex));
            g.fillRoundedRectangle(swatch, 3.0f);
            g.setColour(palette::kBorder.withAlpha(0.7f));
            g.drawRoundedRectangle(swatch.reduced(0.5f), 3.0f, 1.0f);
        }
    }

    void DesignMorphEditorPanel::resized()
    {
        auto bounds = getLocalBounds().reduced(12);

        auto header = bounds.removeFromTop(28);
        backButton_.setBounds(header.removeFromLeft(110));
        header.removeFromLeft(12);
        titleLabel_.setBounds(header.removeFromLeft(180));
        header.removeFromLeft(8);
        subtitleLabel_.setBounds(header);

        bounds.removeFromTop(10);
        auto workspace = bounds.removeFromTop(bounds.getHeight() - 40);

        auto left = workspace.removeFromLeft(layout::kDesignMorphEditorLeftColumnWidth);
        auto right = workspace.removeFromRight(layout::kDesignMorphEditorRightColumnWidth);
        workspace.removeFromRight(8);
        left.removeFromRight(8);

        auto leftHeader = left.removeFromTop(22);
        keyframeHeaderLabel_.setBounds(leftHeader.removeFromLeft(140));
        addKeyframeButton_.setBounds(leftHeader.removeFromRight(52).reduced(0, 1));

        left.removeFromTop(8);
        keyframeList_.setBounds(left.removeFromTop(left.getHeight() - 72));
        left.removeFromTop(8);
        deleteKeyframeButton_.setBounds(left.removeFromTop(28));

        auto center = workspace;
        auto centerHeader = center.removeFromTop(40);
        morphPositionKnob_->setBounds(centerHeader.removeFromRight(88).reduced(4));
        centerHeader.removeFromRight(8);
        recaptureButton_.setBounds(centerHeader.removeFromRight(72).reduced(0, 8));

        colorLabel_.setBounds(center.removeFromTop(12));
        center.removeFromTop(layout::kDesignMorphEditorColorSwatchSize + 8);

        overrideHeaderLabel_.setBounds(center.removeFromTop(12));
        center.removeFromTop(4);
        overrideList_.setBounds(center.removeFromTop(center.getHeight() - 120));
        center.removeFromTop(8);

        curveLabel_.setBounds(center.removeFromTop(12));
        auto curveRow = center.removeFromTop(22);
        const int curveW = (curveRow.getWidth() - static_cast<int>(curveButtons_.size() - 1) * 4)
                           / static_cast<int>(curveButtons_.size());
        for (auto& btn : curveButtons_)
        {
            btn.setBounds(curveRow.removeFromLeft(curveW).reduced(0, 2));
            curveRow.removeFromLeft(4);
        }

        center.removeFromTop(6);
        autoplayLabel_.setBounds(center.removeFromTop(12));
        auto autoplayRow = center.removeFromTop(22);
        const int apW = (autoplayRow.getWidth() - static_cast<int>(autoplayButtons_.size() - 1) * 4)
                        / static_cast<int>(autoplayButtons_.size());
        for (auto& btn : autoplayButtons_)
        {
            btn.setBounds(autoplayRow.removeFromLeft(apW).reduced(0, 2));
            autoplayRow.removeFromLeft(4);
        }

        right.removeFromTop(8);
        timeline_.setBounds(right.removeFromTop(layout::kMorphTimelinePanelHeight));
        right.removeFromTop(8);
        wrapToggle_.setBounds(right.removeFromTop(20));
        disseminationToggle_.setBounds(right.removeFromTop(20));

        auto footer = bounds.removeFromBottom(32);
        openModMatrixButton_.setBounds(footer.removeFromRight(160).reduced(0, 4));
        footer.removeFromRight(12);
        footerHint_.setBounds(footer);
    }

    void DesignMorphEditorPanel::mouseDown(const juce::MouseEvent& event)
    {
        juce::Component::mouseDown(event);
        for (std::size_t i = 0; i < sizeof(kPaletteColors) / sizeof(kPaletteColors[0]); ++i)
        {
            if (colorSwatchBounds(i).contains(event.getPosition()))
            {
                processor_.setMorphKeyframeColor(selectedKeyframe_, kPaletteColors[i]);
                refreshFromPatch();
                return;
            }
        }
    }

} // namespace pw8::plugin::ui
