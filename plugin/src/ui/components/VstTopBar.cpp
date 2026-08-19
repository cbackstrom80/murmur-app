#include "VstTopBar.h"

#include "../theme/BrandingAssets.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        class CompactViewIconButton : public juce::TextButton
        {
        public:
            CompactViewIconButton() : juce::TextButton(juce::String())
            {
                setTooltip("Compact teleprompter (320px performance HUD)");
            }

            void paintButton(juce::Graphics& g, bool highlighted, bool down) override
            {
                juce::TextButton::paintButton(g, highlighted, down);

                auto bounds = getLocalBounds().toFloat().reduced(6.0f);
                const auto centre = bounds.getCentre();
                const float outerR = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.42f;
                const float innerR = outerR * 0.55f;
                const auto colour = getToggleState() ? palette::kAccentWarm : palette::kTextSecondary;

                g.setColour(colour.withAlpha(getToggleState() ? 0.85f : 0.65f));
                g.drawEllipse(centre.x - outerR, centre.y - outerR, outerR * 2.0f, outerR * 2.0f, 1.3f);
                g.drawEllipse(centre.x - innerR, centre.y - innerR, innerR * 2.0f, innerR * 2.0f, 1.0f);

                for (int i = 0; i < 4; ++i)
                {
                    const float a = static_cast<float>(i) * juce::MathConstants<float>::halfPi;
                    const float dotR = 2.0f;
                    const float orbit = outerR * 0.78f;
                    g.fillEllipse(centre.x + std::cos(a) * orbit - dotR, centre.y + std::sin(a) * orbit - dotR,
                                  dotR * 2.0f, dotR * 2.0f);
                }
            }
        };
    } // namespace

    VstTopBar::VstTopBar(MurmurProcessor& processor)
        : arpLauncherChip_(processor)
    {
        compactViewButton_ = std::make_unique<CompactViewIconButton>();

        for (auto* btn : {&playModeTabButton_, &designModeTabButton_, &basicViewButton_, &advancedViewButton_,
                          compactViewButton_.get()})
        {
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(9000);
            btn->setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn->setColour(juce::TextButton::buttonOnColourId, branding::glowColour().withAlpha(0.28f));
            btn->setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            btn->setColour(juce::TextButton::textColourOnId, palette::kTextPrimary);
            addAndMakeVisible(*btn);
        }

        playModeTabButton_.onClick = [this] {
            if (onEditorModeChanged)
                onEditorModeChanged(layout::EditorMode::Play);
        };
        designModeTabButton_.onClick = [this] {
            if (onEditorModeChanged)
                onEditorModeChanged(layout::EditorMode::Design);
        };

        basicViewButton_.onClick = [this] {
            if (onViewModeChanged)
                onViewModeChanged(layout::PlayViewMode::Basic);
        };
        advancedViewButton_.onClick = [this] {
            if (onViewModeChanged)
                onViewModeChanged(layout::PlayViewMode::Advanced);
        };
        compactViewButton_->onClick = [this] {
            if (onViewModeChanged)
                onViewModeChanged(layout::PlayViewMode::Compact);
        };

        arpLauncherChip_.onOpenDrawer = [this] {
            if (onArpRequested)
                onArpRequested();
        };
        addAndMakeVisible(arpLauncherChip_);

        setPlayViewMode(layout::PlayViewMode::Basic);
    }

    void VstTopBar::setEditorMode(layout::EditorMode mode)
    {
        editorMode_ = mode;
        applyViewModeUi();
        resized();
    }

    void VstTopBar::setPlayViewMode(layout::PlayViewMode mode)
    {
        viewMode_ = mode;
        applyViewModeUi();
    }

    void VstTopBar::setCompactWindowMode(bool compactWindow)
    {
        compactWindow_ = compactWindow;
        applyViewModeUi();
        resized();
    }

    void VstTopBar::setUnifiedChromeMode(bool unifiedChrome)
    {
        unifiedChromeMode_ = unifiedChrome;
        repaint();
        resized();
    }

    void VstTopBar::applyViewModeUi()
    {
        playModeTabButton_.setToggleState(editorMode_ == layout::EditorMode::Play, juce::dontSendNotification);
        designModeTabButton_.setToggleState(editorMode_ == layout::EditorMode::Design, juce::dontSendNotification);
        basicViewButton_.setToggleState(viewMode_ == layout::PlayViewMode::Basic, juce::dontSendNotification);
        advancedViewButton_.setToggleState(viewMode_ == layout::PlayViewMode::Advanced, juce::dontSendNotification);
        compactViewButton_->setToggleState(viewMode_ == layout::PlayViewMode::Compact, juce::dontSendNotification);

        const bool compact = compactWindow_ || viewMode_ == layout::PlayViewMode::Compact;
        const bool design = editorMode_ == layout::EditorMode::Design;
        playModeTabButton_.setVisible(unifiedChromeMode_ && !compact);
        designModeTabButton_.setVisible(unifiedChromeMode_ && !compact);
        basicViewButton_.setVisible(!compact && !design);
        advancedViewButton_.setVisible(!compact && !design);
        arpLauncherChip_.setVisible(!compact && !design);
    }

    void VstTopBar::paint(juce::Graphics& g)
    {
        if (unifiedChromeMode_)
            return;

        const auto bounds = getLocalBounds().toFloat();

        g.setColour(palette::kPanelRaised.withAlpha(0.88f));
        g.fillRect(bounds);

        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawHorizontalLine(static_cast<int>(bounds.getY()), bounds.getX() + 8.0f, bounds.getRight() - 8.0f);
        g.drawHorizontalLine(static_cast<int>(bounds.getBottom()) - 1, bounds.getX() + 8.0f, bounds.getRight() - 8.0f);

        g.setColour(palette::kTopHighlight.withAlpha(0.04f));
        g.drawHorizontalLine(static_cast<int>(bounds.getY() + 1.0f), bounds.getX() + 12.0f, bounds.getRight() - 12.0f);
    }

    void VstTopBar::resized()
    {
        auto row = getLocalBounds().reduced(unifiedChromeMode_ ? 0 : 6, unifiedChromeMode_ ? 0 : 3);
        if (playModeTabButton_.isVisible())
        {
            playModeTabButton_.setBounds(row.removeFromLeft(51).reduced(2));
            designModeTabButton_.setBounds(row.removeFromLeft(62).reduced(2));
            row.removeFromLeft(4);
        }

        if (basicViewButton_.isVisible())
            basicViewButton_.setBounds(row.removeFromLeft(64).reduced(2));
        if (advancedViewButton_.isVisible())
            advancedViewButton_.setBounds(row.removeFromLeft(80).reduced(2));
        compactViewButton_->setBounds(row.removeFromLeft(layout::kCompactViewButtonWidth).reduced(2));

        if (arpLauncherChip_.isVisible())
            arpLauncherChip_.setBounds(row.removeFromRight(168).reduced(2, unifiedChromeMode_ ? 8 : 1));
    }

} // namespace pw8::plugin::ui
