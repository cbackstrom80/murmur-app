#include "ModMatrixScreen.h"

#include <algorithm>
#include <array>
#include <vector>

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ModMatrixRow.h"
#include "ModRoutingUi.h"
#include "ModSourceChip.h"
#include "WireframePanel.h"
#include "state/PluginState.h"
#include "wireframe/EnvelopeCurveView.h"
#include "wireframe/LfoWireframeView.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kMatrixRowHeight = 34;
        constexpr int kMatrixHeaderHeight = 20;
        constexpr int kFilterRowHeight = 28;
        constexpr int kModulatorCardMinHeight = 108;
        constexpr int kVisibleModulatorSlots = 3;

        [[nodiscard]] float loadParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id,
                                       float fallback = 0.0f)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        }
    } // namespace

    struct ModMatrixScreen::Impl
    {
        explicit Impl(ModMatrixScreen& owner, PatchworkEightProcessor& processor)
            : owner_(owner), processor_(processor)
        {
            destChoices_ = allModMatrixDestChoices();
        }

        ModMatrixScreen& owner_;
        PatchworkEightProcessor& processor_;
        std::vector<ModMatrixDestChoice> destChoices_;
        std::vector<modulation::ModRoute> disabledRoutes_;

        WireframePanel matrixFrame_{"MOD MATRIX", palette::kAccent};
        juce::TextEditor filterEditor_;
        juce::TextButton addRowButton_{"+"};
        juce::TextButton removeRowButton_{"-"};
        juce::Viewport matrixViewport_;
        juce::Component matrixContent_;
        juce::Component matrixHeader_;

        WireframePanel modulatorsFrame_{"MODULATORS", palette::kAccentWarm};
        juce::Label modulatorHint_{"", "Deep envelope edit: PLAY → ENV tab"};

        struct EnvModuleCard;
        struct LfoModuleCard;

        std::vector<std::unique_ptr<ModMatrixRow>> matrixRows_;
        std::array<std::unique_ptr<EnvModuleCard>, kVisibleModulatorSlots> envCards_{};
        std::array<std::unique_ptr<LfoModuleCard>, kVisibleModulatorSlots> lfoCards_{};

        void rebuildMatrixRows();
        void addRoute();
        void removeRoute();
        [[nodiscard]] bool routeMatchesFilter(const modulation::ModRoute& route) const;
    };

    struct ModMatrixScreen::Impl::EnvModuleCard : public juce::Component, private juce::Timer
    {
        EnvModuleCard(PatchworkEightProcessor& processor, std::size_t envIndex)
            : processor_(processor), envIndex_(envIndex), curveView_(processor.apvts, envIndex)
        {
            titleLabel_.setFont(fonts::label(10.0f));
            titleLabel_.setColour(juce::Label::textColourId, palette::kAccent);
            paramsLabel_.setFont(fonts::value(9.0f));
            paramsLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
            addAndMakeVisible(titleLabel_);
            addAndMakeVisible(paramsLabel_);
            addAndMakeVisible(curveView_);
            const auto source = static_cast<modulation::ModSource>(static_cast<int>(modulation::ModSource::Env1) +
                                                                   static_cast<int>(envIndex_));
            titleLabel_.setText(modSourceLabel(source), juce::dontSendNotification);
            startTimerHz(4);
        }

        ~EnvModuleCard() override { stopTimer(); }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(4);
            titleLabel_.setBounds(bounds.removeFromTop(14));
            bounds.removeFromTop(2);
            paramsLabel_.setBounds(bounds.removeFromBottom(14));
            bounds.removeFromBottom(2);
            curveView_.setBounds(bounds);
        }

        void paint(juce::Graphics& g) override
        {
            WireframePanel::paintFrame(g, getLocalBounds().toFloat(), {}, palette::kAccent.withAlpha(0.35f));
        }

    private:
        void timerCallback() override
        {
            const auto prefix = envelopeParamId(envIndex_, "");
            paramsLabel_.setText("A " + juce::String(loadParam(processor_.apvts, prefix + "Attack", 0.005f), 2) +
                                     "s  D " + juce::String(loadParam(processor_.apvts, prefix + "Decay", 0.2f), 2) +
                                     "s  S " + juce::String(loadParam(processor_.apvts, prefix + "Sustain", 0.7f), 2) +
                                     "  R " + juce::String(loadParam(processor_.apvts, prefix + "Release", 0.3f), 2) + "s",
                                 juce::dontSendNotification);
        }

        PatchworkEightProcessor& processor_;
        std::size_t envIndex_;
        juce::Label titleLabel_;
        juce::Label paramsLabel_;
        wireframe::EnvelopeCurveView curveView_;
    };

    struct ModMatrixScreen::Impl::LfoModuleCard : public juce::Component
    {
        LfoModuleCard(PatchworkEightProcessor& processor, std::size_t lfoIndex)
            : lfoIndex_(lfoIndex), lfoView_(processor.apvts, lfoIndex)
        {
            titleLabel_.setFont(fonts::label(10.0f));
            titleLabel_.setColour(juce::Label::textColourId, palette::kAccent);
            const auto source = static_cast<modulation::ModSource>(static_cast<int>(modulation::ModSource::Lfo1) +
                                                                   static_cast<int>(lfoIndex_));
            titleLabel_.setText(modSourceLabel(source), juce::dontSendNotification);
            addAndMakeVisible(titleLabel_);
            addAndMakeVisible(lfoView_);
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(4);
            titleLabel_.setBounds(bounds.removeFromTop(14));
            bounds.removeFromTop(2);
            lfoView_.setBounds(bounds);
        }

        void paint(juce::Graphics& g) override
        {
            WireframePanel::paintFrame(g, getLocalBounds().toFloat(), {}, palette::kAccent.withAlpha(0.35f));
        }

    private:
        std::size_t lfoIndex_;
        juce::Label titleLabel_;
        wireframe::LfoWireframeView lfoView_;
    };

    bool ModMatrixScreen::Impl::routeMatchesFilter(const modulation::ModRoute& route) const
    {
        const auto filter = filterEditor_.getText().trim().toLowerCase();
        if (filter.isEmpty())
            return true;
        return modSourceLabel(route.source).toLowerCase().contains(filter) ||
               modDestinationLabel(route.destination, route.targetIndex).toLowerCase().contains(filter);
    }

    void ModMatrixScreen::Impl::rebuildMatrixRows()
    {
        matrixRows_.clear();
        matrixContent_.removeAllChildren();

        juce::StringArray seenDestKeys;
        auto addRowForRoute = [&](const modulation::ModRoute& route, bool enabled) {
            if (!route.isActive() && !enabled)
                return;
            if (!routeMatchesFilter(route))
                return;
            const auto key = juce::String(static_cast<int>(route.destination)) + ":" +
                             juce::String(static_cast<int>(route.targetIndex));
            if (seenDestKeys.contains(key))
                return;
            seenDestKeys.add(key);
            auto row = std::make_unique<ModMatrixRow>(processor_, route, enabled, destChoices_, disabledRoutes_,
                                                      [this] { rebuildMatrixRows(); });
            matrixContent_.addAndMakeVisible(*row);
            matrixRows_.push_back(std::move(row));
        };

        for (const auto& route : processor_.getCurrentPatch().layerA.modRoutes)
            addRowForRoute(route, true);
        for (const auto& route : disabledRoutes_)
            addRowForRoute(route, false);

        const int contentHeight = juce::jmax(kMatrixRowHeight, static_cast<int>(matrixRows_.size()) * kMatrixRowHeight);
        matrixContent_.setSize(matrixViewport_.getWidth(), contentHeight);
        for (std::size_t i = 0; i < matrixRows_.size(); ++i)
            matrixRows_[i]->setBounds(0, static_cast<int>(i) * kMatrixRowHeight, matrixContent_.getWidth(),
                                      kMatrixRowHeight);
    }

    void ModMatrixScreen::Impl::addRoute()
    {
        modulation::ModDestination dest = modulation::ModDestination::FilterCutoff;
        for (const auto& choice : destChoices_)
        {
            bool taken = false;
            for (const auto& route : processor_.getCurrentPatch().layerA.modRoutes)
            {
                if (route.isActive() && route.destination == choice.destination)
                {
                    taken = true;
                    break;
                }
            }
            if (!taken)
            {
                dest = choice.destination;
                break;
            }
        }
        assignModRoute(processor_, modulation::ModSource::ModWheel, dest, 0);
        rebuildMatrixRows();
    }

    void ModMatrixScreen::Impl::removeRoute()
    {
        if (matrixRows_.empty())
            return;
        const auto& route = matrixRows_.back()->route();
        processor_.removeModRouteLive(route.source, route.destination, route.targetIndex);
        disabledRoutes_.erase(
            std::remove_if(disabledRoutes_.begin(), disabledRoutes_.end(),
                           [&](const modulation::ModRoute& r) {
                               return r.destination == route.destination && r.targetIndex == route.targetIndex;
                           }),
            disabledRoutes_.end());
        rebuildMatrixRows();
    }

    ModMatrixScreen::ModMatrixScreen(PatchworkEightProcessor& processor)
        : impl_(std::make_unique<Impl>(*this, processor)), processor_(processor)
    {
        auto& i = *impl_;

        i.filterEditor_.setMultiLine(false);
        i.filterEditor_.setReturnKeyStartsNewLine(false);
        i.filterEditor_.setScrollbarsShown(false);
        i.filterEditor_.setTextToShowWhenEmpty("Filter by source or target…", palette::kTextDim);
        i.filterEditor_.setColour(juce::TextEditor::backgroundColourId, palette::kPanelRaised);
        i.filterEditor_.setColour(juce::TextEditor::outlineColourId, palette::kBorder);
        i.filterEditor_.setColour(juce::TextEditor::textColourId, palette::kTextPrimary);
        i.filterEditor_.onTextChange = [this] { impl_->rebuildMatrixRows(); };
        i.matrixFrame_.addAndMakeVisible(i.filterEditor_);

        for (auto* btn : {&i.addRowButton_, &i.removeRowButton_})
        {
            btn->setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn->setColour(juce::TextButton::textColourOffId, palette::kAccent);
            i.matrixFrame_.addAndMakeVisible(*btn);
        }
        i.addRowButton_.onClick = [this] { impl_->addRoute(); };
        i.removeRowButton_.onClick = [this] { impl_->removeRoute(); };

        i.matrixFrame_.addAndMakeVisible(i.matrixHeader_);
        i.matrixViewport_.setViewedComponent(&i.matrixContent_, false);
        i.matrixViewport_.setScrollBarsShown(true, false);
        i.matrixFrame_.addAndMakeVisible(i.matrixViewport_);

        i.modulatorHint_.setFont(fonts::value(9.5f));
        i.modulatorHint_.setColour(juce::Label::textColourId, palette::kTextDim);
        i.modulatorsFrame_.addAndMakeVisible(i.modulatorHint_);

        for (std::size_t slot = 0; slot < kVisibleModulatorSlots; ++slot)
        {
            i.envCards_[slot] = std::make_unique<Impl::EnvModuleCard>(processor_, slot);
            i.lfoCards_[slot] = std::make_unique<Impl::LfoModuleCard>(processor_, slot);
            i.modulatorsFrame_.addAndMakeVisible(*i.envCards_[slot]);
            i.modulatorsFrame_.addAndMakeVisible(*i.lfoCards_[slot]);
        }

        addAndMakeVisible(i.matrixFrame_);
        addAndMakeVisible(i.modulatorsFrame_);

        impl_->rebuildMatrixRows();
        startTimerHz(2);
    }

    ModMatrixScreen::~ModMatrixScreen() = default;

    void ModMatrixScreen::refreshFromPatch()
    {
        impl_->rebuildMatrixRows();
        repaint();
    }

    void ModMatrixScreen::resized()
    {
        auto& i = *impl_;
        auto bounds = getLocalBounds().reduced(2);
        const int modulatorHeight = juce::jmax(kModulatorCardMinHeight * 2 + 48,
                                             static_cast<int>(bounds.getHeight() * 0.38f));
        auto modulatorArea = bounds.removeFromBottom(modulatorHeight);
        bounds.removeFromBottom(6);
        i.matrixFrame_.setBounds(bounds);
        i.modulatorsFrame_.setBounds(modulatorArea);

        auto matrixInner = i.matrixFrame_.getContentBounds().reduced(4);
        auto filterRow = matrixInner.removeFromTop(kFilterRowHeight);
        i.filterEditor_.setBounds(filterRow.removeFromLeft(filterRow.getWidth() - 56).reduced(0, 2));
        i.removeRowButton_.setBounds(filterRow.removeFromRight(24).reduced(1));
        i.addRowButton_.setBounds(filterRow.removeFromRight(24).reduced(1));
        matrixInner.removeFromTop(4);
        i.matrixHeader_.setBounds(matrixInner.removeFromTop(kMatrixHeaderHeight));
        matrixInner.removeFromTop(2);
        i.matrixViewport_.setBounds(matrixInner);
        i.matrixContent_.setSize(matrixInner.getWidth(), static_cast<int>(i.matrixRows_.size()) * kMatrixRowHeight);

        auto modInner = i.modulatorsFrame_.getContentBounds().reduced(4);
        i.modulatorHint_.setBounds(modInner.removeFromBottom(14));
        modInner.removeFromBottom(4);

        const int cardGap = 6;
        const int cardWidth = (modInner.getWidth() - cardGap * (kVisibleModulatorSlots - 1)) / kVisibleModulatorSlots;
        const int halfHeight = (modInner.getHeight() - cardGap) / 2;
        auto envRow = modInner.removeFromTop(halfHeight);
        modInner.removeFromTop(cardGap);
        auto lfoRow = modInner;

        for (std::size_t slot = 0; slot < kVisibleModulatorSlots; ++slot)
        {
            i.envCards_[slot]->setBounds(envRow.removeFromLeft(cardWidth));
            i.lfoCards_[slot]->setBounds(lfoRow.removeFromLeft(cardWidth));
            if (slot + 1 < kVisibleModulatorSlots)
            {
                envRow.removeFromLeft(cardGap);
                lfoRow.removeFromLeft(cardGap);
            }
        }
    }

    void ModMatrixScreen::paint(juce::Graphics& g)
    {
        if (impl_->matrixHeader_.getBounds().isEmpty())
            return;

        auto header = impl_->matrixHeader_.getBounds().toFloat();
        g.setFont(fonts::label(9.0f));
        g.setColour(palette::kTextDim);
        auto bypassCol = header.removeFromLeft(32);
        g.drawText("", bypassCol, juce::Justification::centred);
        header.removeFromLeft(8);
        g.setColour(palette::kAccent.withAlpha(0.85f));
        const int columns = 3;
        const int widgetWidth = header.getWidth() / columns;
        g.drawText("SOURCE", header.removeFromLeft(widgetWidth), juce::Justification::centredLeft);
        g.drawText("DESTINATION", header.removeFromLeft(widgetWidth), juce::Justification::centredLeft);
        g.setColour(palette::kTextDim);
        g.drawText("AMOUNT", header, juce::Justification::centredLeft);
    }

    void ModMatrixScreen::timerCallback()
    {
        for (auto& row : impl_->matrixRows_)
            row->syncFromRoute();
    }

} // namespace pw8::plugin::ui
