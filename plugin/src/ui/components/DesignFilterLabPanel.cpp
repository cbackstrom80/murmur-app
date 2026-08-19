#include "DesignFilterLabPanel.h"

#include "../PlayModeLayout.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ModRoutingUi.h"
#include "ModSourceChip.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] bool isFilterLabDestination(modulation::ModDestination dest) noexcept
        {
            using modulation::ModDestination;
            switch (dest)
            {
                case ModDestination::FilterCutoff:
                case ModDestination::FilterResonance:
                case ModDestination::FilterModeMorph:
                case ModDestination::FilterRouting:
                case ModDestination::FilterDrive:
                    return true;
                default:
                    return false;
            }
        }

        [[nodiscard]] juce::String shortFilterDestLabel(modulation::ModDestination dest) noexcept
        {
            using modulation::ModDestination;
            switch (dest)
            {
                case ModDestination::FilterCutoff: return "CUTOFF";
                case ModDestination::FilterResonance: return "F1 RESO";
                case ModDestination::FilterModeMorph: return "F1 MORPH";
                case ModDestination::FilterRouting: return "ROUTING";
                case ModDestination::FilterDrive: return "F2 DRIVE";
                default: return modDestinationLabel(dest, 0);
            }
        }

        juce::String filterModeToText(float v)
        {
            switch (static_cast<int>(v))
            {
                case 0: return "LP";
                case 1: return "HP";
                case 2: return "BP";
                case 3: return "NOTCH";
                case 4: return "PEAK";
                default: return juce::String(v);
            }
        }

        [[nodiscard]] float loadParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id,
                                      float fallback = 0.0f)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        }
    } // namespace

    class DesignFilterLabPanel::ModRouteListModel : public juce::ListBoxModel
    {
    public:
        explicit ModRouteListModel(DesignFilterLabPanel& owner) : owner_(owner) {}

        int getNumRows() override { return static_cast<int>(owner_.filterModRoutes_.size()); }

        void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected) override
        {
            if (row < 0 || static_cast<std::size_t>(row) >= owner_.filterModRoutes_.size())
                return;

            const auto& route = owner_.filterModRoutes_[static_cast<std::size_t>(row)];
            auto bounds = juce::Rectangle<int>(0, 0, width, height).reduced(2, 1).toFloat();

            g.setColour(selected ? palette::kAccent.withAlpha(0.12f) : palette::kPanelRaised.withAlpha(0.45f));
            g.fillRoundedRectangle(bounds, 4.0f);

            const bool warmDest = route.destination == modulation::ModDestination::FilterDrive;
            const juce::Colour accent = warmDest ? palette::kAccentWarm : palette::kAccent;

            g.setFont(fonts::micro(7.5f));
            g.setColour(palette::kTextPrimary);
            g.drawText(modSourceLabel(route.source), bounds.reduced(6.0f, 0.0f).removeFromLeft(bounds.getWidth() * 0.38f),
                       juce::Justification::centredLeft, true);

            g.setColour(palette::kTextDim);
            g.drawText(fonts::kArrow, bounds.withTrimmedLeft(bounds.getWidth() * 0.38f).withWidth(bounds.getWidth() * 0.08f),
                       juce::Justification::centred, true);

            g.setColour(accent);
            g.drawText(shortFilterDestLabel(route.destination),
                       bounds.reduced(6.0f, 0.0f).withTrimmedLeft(bounds.getWidth() * 0.46f).withTrimmedRight(56.0f),
                       juce::Justification::centredLeft, true);

            g.setColour(accent.withAlpha(0.9f));
            g.drawText(formatModRouteAmount(route.destination, route.amount),
                       bounds.reduced(6.0f, 0.0f).removeFromRight(52.0f), juce::Justification::centredRight, true);
        }

    private:
        DesignFilterLabPanel& owner_;
    };

    DesignFilterLabPanel::DesignFilterLabPanel(PatchworkEightProcessor& processor,
                                                 ModAssignmentController& assignmentController)
        : processor_(processor),
          assignmentController_(assignmentController),
          routingWireframe_(processor.apvts),
          f1Panel_(juce::String("Filter 1") + fonts::kDash + juce::String("State Variable"), palette::kAccent),
          f2Panel_(juce::String("Filter 2") + fonts::kDash + juce::String("Ladder Drive"), palette::kAccentWarm),
          routingHeroPanel_("Signal Pipeline", palette::kAccent)
    {
        routingWireframe_.setStackedStatesLayout(true);

        modRouteModel_ = std::make_unique<ModRouteListModel>(*this);
        modRouteList_.setModel(modRouteModel_.get());
        modRouteList_.setRowHeight(layout::kDesignFilterLabModRouteRowHeight);
        modRouteList_.setOutlineThickness(0);

        backButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        backButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        backButton_.onClick = [this] {
            if (onClosed)
                onClosed();
        };
        addAndMakeVisible(backButton_);

        titleLabel_.setText("FILTER LAB", juce::dontSendNotification);
        titleLabel_.setFont(fonts::label(12.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kAccent);
        addAndMakeVisible(titleLabel_);

        subtitleLabel_.setText("BLADES DUAL FILTER" + juce::String(fonts::kSep) + "ROUTING MORPH" + juce::String(fonts::kSep)
                                   + "MOD MATRIX",
                               juce::dontSendNotification);
        subtitleLabel_.setFont(fonts::label(7.0f));
        subtitleLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(subtitleLabel_);

        addAndMakeVisible(routingWireframe_);
        addAndMakeVisible(f1Panel_);
        addAndMakeVisible(f2Panel_);
        addAndMakeVisible(routingHeroPanel_);
        addAndMakeVisible(modRoutesPanel_);
        modRoutesPanel_.addAndMakeVisible(modRouteList_);

        f1ActiveBadge_.setText("ACTIVE", juce::dontSendNotification);
        f1ActiveBadge_.setFont(fonts::label(7.0f));
        f1ActiveBadge_.setColour(juce::Label::textColourId, palette::kAccent);
        f1ActiveBadge_.setJustificationType(juce::Justification::centredRight);
        f1Panel_.addAndMakeVisible(f1ActiveBadge_);

        f2WarmthBadge_.setText("WARMTH", juce::dontSendNotification);
        f2WarmthBadge_.setFont(fonts::label(7.0f));
        f2WarmthBadge_.setColour(juce::Label::textColourId, palette::kAccentWarm);
        f2WarmthBadge_.setJustificationType(juce::Justification::centredRight);
        f2Panel_.addAndMakeVisible(f2WarmthBadge_);

        f1Panel_.addAndMakeVisible(f1EnabledButton_);
        f2Panel_.addAndMakeVisible(f2EnabledButton_);

        static constexpr const char* kModeLabels[] = {"LP", "BP", "HP", "NOTCH"};
        static constexpr int kModeValues[] = {0, 2, 1, 3};
        for (std::size_t i = 0; i < f1ModePills_.size(); ++i)
        {
            styleModePill(f1ModePills_[i], i == 0);
            f1ModePills_[i].setButtonText(kModeLabels[i]);
            const int modeValue = kModeValues[i];
            f1ModePills_[i].onClick = [this, modeValue] {
                if (auto* param = processor_.apvts.getParameter(juce::String(kFilterIdPrefix) + "Mode"))
                    param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(modeValue)));
                refreshModePills();
            };
            f1Panel_.addAndMakeVisible(f1ModePills_[i]);
        }

        auto& apvts = processor_.apvts;
        const juce::String f1 = juce::String(kFilterIdPrefix);
        const juce::String f2 = juce::String(kFilter2IdPrefix);

        f1EnabledAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, f1 + "Enabled", f1EnabledButton_);
        f2EnabledAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, f2 + "Enabled", f2EnabledButton_);

        f1ModeMorphKnob_ = std::make_unique<GlowKnob>(apvts, f1 + "ModeMorph", "Morph", filterModeToText);
        f1ToneKnob_ = std::make_unique<ConcentricGlowKnob>(apvts, f1 + "Resonance", f1 + "CutoffHz", "Reso", "Cutoff");
        f1KeyTrackKnob_ = std::make_unique<GlowKnob>(apvts, f1 + "KeyTrack", "Key Trk");
        f1ToneKnob_->setInnerAccentColour(palette::kAccent);
        f1ToneKnob_->enableInnerModulationTarget(processor_, modulation::ModDestination::FilterResonance, 0);
        f1ToneKnob_->enableOuterModulationTarget(processor_, modulation::ModDestination::FilterCutoff, 0);
        f1ToneKnob_->setModAssignmentController(&assignmentController_);
        f1ModeMorphKnob_->enableModulationTarget(processor_, modulation::ModDestination::FilterModeMorph, 0);
        f1ModeMorphKnob_->setModAssignmentController(&assignmentController_);

        f2CutoffOffsetKnob_ = std::make_unique<GlowKnob>(apvts, f2 + "CutoffOffsetSemis", "F2 Off");
        f2ResonanceKnob_ = std::make_unique<GlowKnob>(apvts, f2 + "Resonance", "F2 Reso");
        f2DriveKnob_ = std::make_unique<GlowKnob>(apvts, f2 + "Drive", "Drive");
        f2KeyTrackKnob_ = std::make_unique<GlowKnob>(apvts, f2 + "KeyTrack", "Key Trk");
        f2DriveKnob_->enableModulationTarget(processor_, modulation::ModDestination::FilterDrive, 0);
        f2DriveKnob_->setModAssignmentController(&assignmentController_);
        f2DriveKnob_->setDeckedStyle(true, GlowKnob::DeckedKnobSize::Medium);

        f1ModeMorphKnob_->applyFigmaContext(figma::KnobContext::DesignLabGrid);
        f1ToneKnob_->applyFigmaContext(figma::KnobContext::DesignLabGrid);
        f1KeyTrackKnob_->applyFigmaContext(figma::KnobContext::DesignLabGrid);
        f2CutoffOffsetKnob_->applyFigmaContext(figma::KnobContext::DesignLabGrid);
        f2ResonanceKnob_->applyFigmaContext(figma::KnobContext::DesignLabGrid);
        f2KeyTrackKnob_->applyFigmaContext(figma::KnobContext::DesignLabGrid);
        f2DriveKnob_->applyFigmaContext(figma::KnobContext::PanelGridMedium);

        routingMorphKnob_ = std::make_unique<GlowKnob>(apvts, kFilterRoutingId, "Route");
        routingMorphKnob_->applyFigmaContext(figma::KnobContext::OperatorHero);
        routingMorphKnob_->enableModulationTarget(processor_, modulation::ModDestination::FilterRouting, 0);
        routingMorphKnob_->setModAssignmentController(&assignmentController_);
        routingHeroPanel_.addAndMakeVisible(*routingMorphKnob_);

        routingMorphLabel_.setText("ROUTING MORPH", juce::dontSendNotification);
        routingMorphLabel_.setFont(fonts::label(8.0f));
        routingMorphLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        routingHeroPanel_.addAndMakeVisible(routingMorphLabel_);

        routingMorphValueLabel_.setFont(fonts::label(7.0f));
        routingMorphValueLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        routingHeroPanel_.addAndMakeVisible(routingMorphValueLabel_);

        f1Panel_.addAndMakeVisible(*f1ModeMorphKnob_);
        f1Panel_.addAndMakeVisible(*f1ToneKnob_);
        f1Panel_.addAndMakeVisible(*f1KeyTrackKnob_);
        for (auto* k : {f2CutoffOffsetKnob_.get(), f2ResonanceKnob_.get(), f2DriveKnob_.get(), f2KeyTrackKnob_.get()})
            f2Panel_.addAndMakeVisible(*k);

        openModMatrixButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        openModMatrixButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        openModMatrixButton_.onClick = [this] {
            if (onOpenModMatrix)
                onOpenModMatrix();
        };
        addAndMakeVisible(openModMatrixButton_);

        openPlayFilterButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        openPlayFilterButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        openPlayFilterButton_.onClick = [this] {
            if (onOpenPlayFilter)
                onOpenPlayFilter();
        };
        addAndMakeVisible(openPlayFilterButton_);

        footerHint_.setText("Routing 0 = serial F1" + juce::String(fonts::kArrow) + "F2" + juce::String(fonts::kSep)
                                + "0.5 = parallel" + juce::String(fonts::kSep) + "1 = crossfade",
                            juce::dontSendNotification);
        footerHint_.setFont(fonts::micro(8.0f));
        footerHint_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(footerHint_);

        startTimerHz(12);
    }

    DesignFilterLabPanel::~DesignFilterLabPanel()
    {
        stopTimer();
        modRouteList_.setModel(nullptr);
    }

    void DesignFilterLabPanel::showOverlay()
    {
        setVisible(true);
        refreshFromPatch();
    }

    void DesignFilterLabPanel::dismiss() { setVisible(false); }

    void DesignFilterLabPanel::setEmbeddedInDesignMode(bool embedded)
    {
        embeddedInDesignMode_ = embedded;
        backButton_.setButtonText(embedded ? "← ENGINE" : "← DESIGN");
    }

    void DesignFilterLabPanel::refreshFromPatch()
    {
        refreshModePills();
        rebuildModRouteList();
        repaint();
    }

    void DesignFilterLabPanel::styleModePill(juce::TextButton& btn, bool active)
    {
        btn.setClickingTogglesState(false);
        btn.setColour(juce::TextButton::buttonColourId, active ? palette::kAccent.withAlpha(0.14f) : palette::kPanelRaised);
        btn.setColour(juce::TextButton::textColourOffId, active ? palette::kAccent : palette::kTextDim);
    }

    void DesignFilterLabPanel::refreshModePills()
    {
        const float mode = loadParam(processor_.apvts, juce::String(kFilterIdPrefix) + "Mode");
        static constexpr int kModeValues[] = {0, 2, 1, 3};
        for (std::size_t i = 0; i < f1ModePills_.size(); ++i)
            styleModePill(f1ModePills_[i], static_cast<int>(mode) == kModeValues[i]);
    }

    void DesignFilterLabPanel::rebuildModRouteList()
    {
        filterModRoutes_.clear();
        for (const auto& route : processor_.getCurrentPatch().layerA.modRoutes)
        {
            if (route.source != modulation::ModSource::None && isFilterLabDestination(route.destination))
                filterModRoutes_.push_back(route);
        }
        modRouteList_.updateContent();
    }

    void DesignFilterLabPanel::timerCallback()
    {
        const float routing = loadParam(processor_.apvts, kFilterRoutingId);
        juce::String modeLabel = "SERIAL";
        if (routing >= 0.95f)
            modeLabel = "CROSSFADE";
        else if (routing > 0.05f && routing < 0.95f)
            modeLabel = routing <= 0.5f ? "PARALLEL" : "MID-CROSS";

        routingMorphValueLabel_.setText(
            modeLabel + juce::String(fonts::kSep) + juce::String(static_cast<int>(routing * 100.0f)) + "%",
            juce::dontSendNotification);
        const bool f1On = loadParam(processor_.apvts, juce::String(kFilterIdPrefix) + "Enabled") >= 0.5f;
        const bool f2On = loadParam(processor_.apvts, juce::String(kFilter2IdPrefix) + "Enabled") >= 0.5f;
        f1ActiveBadge_.setVisible(f1On);
        f2WarmthBadge_.setVisible(f2On);

        rebuildModRouteList();
        repaint(f1SpectrumBounds_);
        repaint(f2DriveBounds_);
    }

    void DesignFilterLabPanel::paintPanelCard(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        draw::fillRecessedRoundedRect(g, bounds.toFloat(), 6.0f);
        g.setColour(palette::kBorder.withAlpha(0.65f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 6.0f, 1.0f);
    }

    void DesignFilterLabPanel::paintF1Spectrum(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        paintPanelCard(g, bounds);
        auto inner = bounds.reduced(12).toFloat();
        auto header = inner.removeFromTop(12.0f);
        g.setFont(fonts::label(8.0f));
        g.setColour(palette::kTextDim);
        g.drawText("F1 SVF SPECTRAL MAGNITUDE", header, juce::Justification::centredLeft, true);

        const float cutoff = loadParam(processor_.apvts, juce::String(kFilterIdPrefix) + "CutoffHz", 1000.0f);
        g.setColour(palette::kAccent);
        g.drawText("CUTOFF: " + juce::String(cutoff / 1000.0f, 2) + " kHz", header, juce::Justification::centredRight,
                   true);

        inner = inner.reduced(0.0f, 4.0f);
        g.setColour(palette::kBackgroundBottom);
        g.fillRoundedRectangle(inner, 4.0f);
        g.setColour(palette::kBorder.withAlpha(0.5f));
        g.drawRoundedRectangle(inner.reduced(0.5f), 4.0f, 1.0f);

        const float reso = loadParam(processor_.apvts, juce::String(kFilterIdPrefix) + "Resonance", 0.2f);
        juce::Path curve;
        const int steps = 48;
        for (int i = 0; i <= steps; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(steps);
            const float freq = 20.0f + t * t * 20000.0f;
            const float norm = freq / juce::jmax(20.0f, cutoff);
            float mag = 1.0f / (1.0f + norm * norm);
            if (std::abs(std::log2(norm)) < 0.08f)
                mag += reso * 2.5f;
            const float x = inner.getX() + t * inner.getWidth();
            const float y = inner.getBottom() - juce::jlimit(0.0f, 1.0f, mag * 0.55f) * inner.getHeight();
            if (i == 0)
                curve.startNewSubPath(x, y);
            else
                curve.lineTo(x, y);
        }
        g.setColour(palette::kAccent.withAlpha(0.85f));
        g.strokePath(curve, juce::PathStrokeType(1.6f));
    }

    void DesignFilterLabPanel::paintF2DriveMeter(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        paintPanelCard(g, bounds);
        auto inner = bounds.reduced(12).toFloat();
        auto header = inner.removeFromTop(12.0f);
        g.setFont(fonts::label(8.0f));
        g.setColour(palette::kTextDim);
        g.drawText("F2 DRIVE SATURATION LEVEL", header, juce::Justification::centredLeft, true);

        const float drive = loadParam(processor_.apvts, juce::String(kFilter2IdPrefix) + "Drive");
        g.setColour(palette::kAccentWarm);
        g.drawText(drive > 0.05f ? "SOFT CLIP ACTIVE" : "CLEAN", header, juce::Justification::centredRight, true);

        auto meter = inner.reduced(0.0f, 8.0f);
        g.setColour(palette::kBackgroundBottom);
        g.fillRoundedRectangle(meter, 4.0f);

        const int segments = 24;
        const float fill = juce::jlimit(0.0f, 1.0f, drive);
        const float segW = (meter.getWidth() - static_cast<float>(segments - 1)) / static_cast<float>(segments);
        for (int i = 0; i < segments; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(segments - 1);
            auto seg = juce::Rectangle<float>(meter.getX() + static_cast<float>(i) * (segW + 1.0f), meter.getY(),
                                              segW, meter.getHeight());
            const bool lit = t <= fill;
            g.setColour(lit ? palette::kAccentWarm.withAlpha(0.35f + t * 0.65f) : palette::kPanelRaised.withAlpha(0.8f));
            g.fillRoundedRectangle(seg, 2.0f);
        }
    }

    bool DesignFilterLabPanel::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::escapeKey)
        {
            if (onClosed)
                onClosed();
            return true;
        }
        return false;
    }

    void DesignFilterLabPanel::paint(juce::Graphics& g)
    {
        g.fillAll(palette::kBackgroundTop);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));
    }

    void DesignFilterLabPanel::paintOverChildren(juce::Graphics& g)
    {
        if (!f1SpectrumBounds_.isEmpty())
            paintF1Spectrum(g, f1SpectrumBounds_);
        if (!f2DriveBounds_.isEmpty())
            paintF2DriveMeter(g, f2DriveBounds_);
    }

    void DesignFilterLabPanel::resized()
    {
        auto bounds = getLocalBounds().reduced(layout::kDesignFilterLabPageOuterMargin);

        auto header = bounds.removeFromTop(layout::kDesignFilterLabHeaderHeight);
        backButton_.setBounds(header.removeFromLeft(110));
        header.removeFromLeft(12);
        titleLabel_.setBounds(header.removeFromLeft(120));
        header.removeFromLeft(8);
        subtitleLabel_.setBounds(header);

        bounds.removeFromTop(layout::kDesignFilterLabPageSectionGap);
        auto footer = bounds.removeFromBottom(layout::kDesignFilterLabFooterHeight);
        openModMatrixButton_.setBounds(footer.removeFromLeft(150).reduced(0, 6));
        footer.removeFromLeft(12);
        footerHint_.setBounds(footer.removeFromLeft(juce::jmax(220, footer.getWidth() / 2)));
        openPlayFilterButton_.setBounds(footer.removeFromRight(180).reduced(0, 6));

        bounds.removeFromBottom(layout::kDesignFilterLabPageSectionGap);

        auto right = bounds.removeFromRight(layout::kDesignFilterLabRightColumnWidth);
        bounds.removeFromRight(layout::kDesignFilterLabPageSectionGap);
        auto center = bounds.removeFromLeft(layout::kDesignFilterLabCenterColumnWidth);
        bounds.removeFromRight(layout::kDesignFilterLabPageSectionGap);
        auto left = bounds;

        routingWireframe_.setBounds(left.removeFromTop(168).reduced(0, 2));
        left.removeFromTop(layout::kDesignFilterLabPageSectionGap);
        f1SpectrumBounds_ = left.removeFromTop(168);
        left.removeFromTop(layout::kDesignFilterLabPageSectionGap);
        f2DriveBounds_ = left;

        modRoutesPanel_.setBounds(right);
        {
            auto content = modRoutesPanel_.getContentBounds().reduced(4, 0);
            modRouteList_.setBounds(content);
        }

        auto f1Area = center.removeFromTop((center.getHeight() - layout::kDesignFilterLabRoutingHeroHeight
                                            - layout::kDesignFilterLabPageSectionGap * 2)
                                           / 2);
        center.removeFromTop(layout::kDesignFilterLabPageSectionGap);
        auto f2Area = center.removeFromTop((center.getHeight() - layout::kDesignFilterLabRoutingHeroHeight) / 2);
        center.removeFromTop(layout::kDesignFilterLabPageSectionGap);
        routingHeroPanel_.setBounds(center);

        f1Panel_.setBounds(f1Area);
        f2Panel_.setBounds(f2Area);

        {
            auto content = f1Panel_.getContentBounds().reduced(4, 0);
            auto headerRow = content.removeFromTop(14);
            f1EnabledButton_.setBounds(headerRow.removeFromRight(28).withSizeKeepingCentre(28, 28));
            f1ActiveBadge_.setBounds(headerRow);
            auto modeRow = content.removeFromTop(22);
            const int pillW = (modeRow.getWidth() - static_cast<int>(f1ModePills_.size() - 1) * 4)
                              / static_cast<int>(f1ModePills_.size());
            for (auto& pill : f1ModePills_)
            {
                pill.setBounds(modeRow.removeFromLeft(pillW).reduced(0, 2));
                modeRow.removeFromLeft(4);
            }
            content.removeFromTop(6);
            const int knobW = content.getWidth() / 3;
            f1ModeMorphKnob_->setBounds(content.removeFromLeft(knobW).reduced(4));
            f1ToneKnob_->setBounds(content.removeFromLeft(knobW).reduced(4));
            f1KeyTrackKnob_->setBounds(content.removeFromLeft(knobW).reduced(4));
        }

        {
            auto content = f2Panel_.getContentBounds().reduced(4, 0);
            auto headerRow = content.removeFromTop(14);
            f2EnabledButton_.setBounds(headerRow.removeFromRight(28).withSizeKeepingCentre(28, 28));
            f2WarmthBadge_.setBounds(headerRow);
            content.removeFromTop(4);
            const int knobW = content.getWidth() / 4;
            f2CutoffOffsetKnob_->setBounds(content.removeFromLeft(knobW).reduced(4));
            f2ResonanceKnob_->setBounds(content.removeFromLeft(knobW).reduced(4));
            f2DriveKnob_->setBounds(content.removeFromLeft(knobW).reduced(4));
            f2DriveKnob_->applyFigmaContext(figma::KnobContext::PanelGridMedium);
            f2KeyTrackKnob_->setBounds(content.removeFromLeft(knobW).reduced(4));
        }

        {
            auto content = routingHeroPanel_.getContentBounds().reduced(8, 4);
            routingMorphLabel_.setBounds(content.removeFromTop(14));
            auto heroRow = content;
            routingMorphKnob_->setBounds(heroRow.withSizeKeepingCentre(layout::kDesignFilterLabHeroKnobSize + 16,
                                                                       layout::kDesignFilterLabHeroKnobSize + 24));
            routingMorphValueLabel_.setBounds(content.removeFromBottom(14));
        }
    }

} // namespace pw8::plugin::ui
