#include "ModMatrixScreen.h"

#include <algorithm>
#include <array>
#include <vector>

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
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
        constexpr int kMatrixRowHeight = 26;
        constexpr int kMatrixHeaderHeight = 20;
        constexpr int kFilterRowHeight = 28;
        constexpr int kModulatorCardMinHeight = 108;
        constexpr int kVisibleModulatorSlots = 3;

        struct DestChoice
        {
            modulation::ModDestination destination = modulation::ModDestination::None;
            bool usesTargetIndex = false;
            juce::String label;
        };

        [[nodiscard]] std::vector<DestChoice> allDestChoices()
        {
            return {
                {modulation::ModDestination::FilterCutoff, false, "Global Filter Cutoff"},
                {modulation::ModDestination::FilterResonance, false, "Global Filter Resonance"},
                {modulation::ModDestination::OperatorFilterCutoff, true, "Engine Filter Cutoff"},
                {modulation::ModDestination::OperatorFilterResonance, true, "Engine Filter Resonance"},
                {modulation::ModDestination::OperatorLevel, true, "Operator Level"},
                {modulation::ModDestination::OperatorWavetablePosition, true, "WT Position"},
                {modulation::ModDestination::OperatorWavetableBend, true, "WT Bend"},
                {modulation::ModDestination::OperatorWavetableAsymmetry, true, "WT Asymmetry"},
                {modulation::ModDestination::OperatorWavetableSyncRatio, true, "WT Sync Ratio"},
                {modulation::ModDestination::OperatorWavetableSyncAmount, true, "WT Sync Amt"},
                {modulation::ModDestination::OperatorWavetableFormant, true, "WT Formant"},
                {modulation::ModDestination::Pan, false, "Layer Pan"},
            };
        }

        [[nodiscard]] std::vector<modulation::ModSource> allModSources()
        {
            return {
                modulation::ModSource::ModWheel,       modulation::ModSource::Expression,
                modulation::ModSource::Velocity,       modulation::ModSource::ChannelPressure,
                modulation::ModSource::PolyAftertouch, modulation::ModSource::MpeSlide,
                modulation::ModSource::Lfo1,           modulation::ModSource::Lfo2,
                modulation::ModSource::Lfo3,           modulation::ModSource::Lfo4,
                modulation::ModSource::Lfo5,           modulation::ModSource::Lfo6,
                modulation::ModSource::Lfo7,           modulation::ModSource::Lfo8,
                modulation::ModSource::Env1,           modulation::ModSource::Env2,
                modulation::ModSource::Env3,           modulation::ModSource::Env4,
                modulation::ModSource::Env5,           modulation::ModSource::Env6,
                modulation::ModSource::Env7,           modulation::ModSource::Env8,
                modulation::ModSource::Macro1,         modulation::ModSource::Macro2,
                modulation::ModSource::Macro3,         modulation::ModSource::Macro4,
                modulation::ModSource::Macro5,         modulation::ModSource::Macro6,
                modulation::ModSource::Macro7,         modulation::ModSource::Macro8,
            };
        }

        [[nodiscard]] float loadParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id,
                                       float fallback = 0.0f)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        }

        [[nodiscard]] juce::String formatAmountDisplay(modulation::ModDestination destination, float amount)
        {
            const auto range = modAmountRangeFor(destination);
            if (range.min >= -1.01f && range.max <= 1.01f)
            {
                const float pct = juce::jlimit(-100.0f, 100.0f, amount * 100.0f);
                const auto sign = pct >= 0.0f ? "+" : "";
                return sign + juce::String(pct, 1) + "%";
            }
            return formatModRouteAmount(destination, amount);
        }

        void styleOrangeCombo(juce::ComboBox& combo)
        {
            combo.setColour(juce::ComboBox::backgroundColourId, palette::kPanelRaised);
            combo.setColour(juce::ComboBox::outlineColourId, palette::kAccentWarmDim);
            combo.setColour(juce::ComboBox::textColourId, palette::kAccentWarm);
            combo.setColour(juce::ComboBox::arrowColourId, palette::kAccentWarm);
        }
    } // namespace

    struct ModMatrixScreen::Impl
    {
        explicit Impl(ModMatrixScreen& owner, PatchworkEightProcessor& processor)
            : owner_(owner), processor_(processor)
        {
            destChoices_ = allDestChoices();
        }

        ModMatrixScreen& owner_;
        PatchworkEightProcessor& processor_;
        std::vector<DestChoice> destChoices_;
        std::vector<modulation::ModRoute> disabledRoutes_;

        WireframePanel matrixFrame_{"MOD MATRIX", palette::kAccentWarm};
        juce::TextEditor filterEditor_;
        juce::TextButton addRowButton_{"+"};
        juce::TextButton removeRowButton_{"-"};
        juce::Viewport matrixViewport_;
        juce::Component matrixContent_;
        juce::Component matrixHeader_;

        WireframePanel modulatorsFrame_{"MODULATORS", palette::kAccentWarm};
        juce::Label modulatorHint_{"", "Deep envelope edit: PLAY → ENV tab"};

        struct MatrixRow;
        struct EnvModuleCard;
        struct LfoModuleCard;

        std::vector<std::unique_ptr<MatrixRow>> matrixRows_;
        std::array<std::unique_ptr<EnvModuleCard>, kVisibleModulatorSlots> envCards_{};
        std::array<std::unique_ptr<LfoModuleCard>, kVisibleModulatorSlots> lfoCards_{};

        void rebuildMatrixRows();
        void addRoute();
        void removeRoute();
        [[nodiscard]] bool routeMatchesFilter(const modulation::ModRoute& route) const;
    };

    struct ModMatrixScreen::Impl::MatrixRow : public juce::Component
    {
        MatrixRow(Impl& impl, const modulation::ModRoute& route, bool enabled) : impl_(impl), route_(route), enabled_(enabled)
        {
            onToggle_.setToggleState(enabled, juce::dontSendNotification);
            onToggle_.setColour(juce::ToggleButton::tickColourId, palette::kAccent);
            onToggle_.setColour(juce::ToggleButton::tickDisabledColourId, palette::kTextDim);
            onToggle_.onClick = [this] { handleToggle(); };
            addAndMakeVisible(onToggle_);

            for (const auto source : allModSources())
                sourceCombo_.addItem(modSourceLabel(source), static_cast<int>(source));
            for (int i = 0; i < static_cast<int>(impl_.destChoices_.size()); ++i)
                destCombo_.addItem(impl_.destChoices_[static_cast<std::size_t>(i)].label, i + 1);
            for (int i = 0; i < static_cast<int>(core::kNodesPerLayer); ++i)
                targetCombo_.addItem("Op " + juce::String(i), i + 1);

            styleOrangeCombo(sourceCombo_);
            styleOrangeCombo(destCombo_);
            styleOrangeCombo(targetCombo_);
            sourceCombo_.onChange = [this] { commitSourceChange(); };
            destCombo_.onChange = [this] { commitDestChange(); };
            targetCombo_.onChange = [this] { commitTargetChange(); };
            addAndMakeVisible(sourceCombo_);
            addAndMakeVisible(destCombo_);
            addAndMakeVisible(targetCombo_);

            amountSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
            amountSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            amountSlider_.setColour(juce::Slider::trackColourId, palette::kPanelRaised);
            amountSlider_.setColour(juce::Slider::backgroundColourId, palette::kPanel);
            amountSlider_.setColour(juce::Slider::thumbColourId, palette::kAccentWarm);
            amountSlider_.onValueChange = [this] { commitAmountChange(); };
            addAndMakeVisible(amountSlider_);

            amountLabel_.setFont(fonts::label(10.0f));
            amountLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
            amountLabel_.setJustificationType(juce::Justification::centredRight);
            addAndMakeVisible(amountLabel_);

            syncFromRoute();
        }

        void syncFromRoute()
        {
            route_ = findLiveRoute().value_or(route_);
            enabled_ = route_.isActive();
            onToggle_.setToggleState(enabled_, juce::dontSendNotification);

            if (sourceCombo_.indexOfItemId(static_cast<int>(route_.source)) >= 0)
                sourceCombo_.setSelectedId(static_cast<int>(route_.source), juce::dontSendNotification);

            for (int i = 0; i < static_cast<int>(impl_.destChoices_.size()); ++i)
            {
                if (impl_.destChoices_[static_cast<std::size_t>(i)].destination == route_.destination)
                {
                    destCombo_.setSelectedId(i + 1, juce::dontSendNotification);
                    break;
                }
            }
            targetCombo_.setSelectedId(static_cast<int>(route_.targetIndex) + 1, juce::dontSendNotification);
            updateTargetVisibility();
            updateAmountSlider();
            amountLabel_.setText(formatAmountDisplay(route_.destination, route_.amount), juce::dontSendNotification);
        }

        [[nodiscard]] const modulation::ModRoute& route() const noexcept { return route_; }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(2, 1);
            onToggle_.setBounds(bounds.removeFromLeft(28));
            bounds.removeFromLeft(4);
            sourceCombo_.setBounds(bounds.removeFromLeft(118).reduced(0, 2));
            bounds.removeFromLeft(4);
            destCombo_.setBounds(bounds.removeFromLeft(148).reduced(0, 2));
            if (targetCombo_.isVisible())
            {
                bounds.removeFromLeft(4);
                targetCombo_.setBounds(bounds.removeFromLeft(52).reduced(0, 2));
            }
            bounds.removeFromLeft(6);
            amountLabel_.setBounds(bounds.removeFromRight(58));
            bounds.removeFromRight(4);
            amountSlider_.setBounds(bounds.reduced(0, 6));
        }

    private:
        [[nodiscard]] std::optional<modulation::ModRoute> findLiveRoute() const
        {
            for (const auto& live : impl_.processor_.getCurrentPatch().layerA.modRoutes)
            {
                if (live.destination == route_.destination && live.targetIndex == route_.targetIndex)
                    return live;
            }
            for (const auto& disabled : impl_.disabledRoutes_)
            {
                if (disabled.destination == route_.destination && disabled.targetIndex == route_.targetIndex)
                    return disabled;
            }
            return std::nullopt;
        }

        void updateTargetVisibility()
        {
            const int destIndex = destCombo_.getSelectedId() - 1;
            const bool usesTarget =
                destIndex >= 0 && destIndex < static_cast<int>(impl_.destChoices_.size()) &&
                impl_.destChoices_[static_cast<std::size_t>(destIndex)].usesTargetIndex;
            targetCombo_.setVisible(usesTarget);
        }

        void updateAmountSlider()
        {
            const auto range = modAmountRangeFor(route_.destination);
            amountSlider_.setRange(range.min, range.max, (range.max - range.min) / 200.0);
            amountSlider_.setValue(route_.amount, juce::dontSendNotification);
        }

        void handleToggle()
        {
            if (onToggle_.getToggleState())
            {
                for (auto it = impl_.disabledRoutes_.begin(); it != impl_.disabledRoutes_.end(); ++it)
                {
                    if (it->destination == route_.destination && it->targetIndex == route_.targetIndex)
                    {
                        const auto restored = *it;
                        impl_.disabledRoutes_.erase(it);
                        impl_.processor_.setOrReplaceModRouteLive(restored.source, restored.destination,
                                                                    restored.targetIndex, restored.amount,
                                                                    restored.scope);
                        route_ = restored;
                        enabled_ = true;
                        syncFromRoute();
                        return;
                    }
                }
            }
            else if (route_.isActive())
            {
                impl_.disabledRoutes_.push_back(route_);
                impl_.processor_.removeModRouteLive(route_.source, route_.destination, route_.targetIndex);
                enabled_ = false;
            }
        }

        void commitSourceChange()
        {
            const auto newSource = static_cast<modulation::ModSource>(sourceCombo_.getSelectedId());
            if (newSource == modulation::ModSource::None || newSource == route_.source)
                return;
            if (route_.isActive())
                impl_.processor_.removeModRouteLive(route_.source, route_.destination, route_.targetIndex);
            route_.source = newSource;
            if (enabled_)
                impl_.processor_.setOrReplaceModRouteLive(route_.source, route_.destination, route_.targetIndex,
                                                            route_.amount, route_.scope);
            syncFromRoute();
        }

        void commitDestChange()
        {
            const int destIndex = destCombo_.getSelectedId() - 1;
            if (destIndex < 0 || destIndex >= static_cast<int>(impl_.destChoices_.size()))
                return;
            const auto& choice = impl_.destChoices_[static_cast<std::size_t>(destIndex)];
            if (choice.destination == route_.destination)
            {
                updateTargetVisibility();
                return;
            }
            if (route_.isActive())
                impl_.processor_.removeModRouteLive(route_.source, route_.destination, route_.targetIndex);
            impl_.disabledRoutes_.erase(
                std::remove_if(impl_.disabledRoutes_.begin(), impl_.disabledRoutes_.end(),
                               [&](const modulation::ModRoute& r) {
                                   return r.destination == route_.destination && r.targetIndex == route_.targetIndex;
                               }),
                impl_.disabledRoutes_.end());
            route_.destination = choice.destination;
            if (!choice.usesTargetIndex)
                route_.targetIndex = 0;
            route_.amount = defaultModAmountFor(route_.destination);
            if (enabled_)
                impl_.processor_.setOrReplaceModRouteLive(route_.source, route_.destination, route_.targetIndex,
                                                            route_.amount, route_.scope);
            updateTargetVisibility();
            updateAmountSlider();
            impl_.rebuildMatrixRows();
        }

        void commitTargetChange()
        {
            const auto newTarget = static_cast<std::uint8_t>(juce::jlimit(
                0, static_cast<int>(core::kNodesPerLayer) - 1, targetCombo_.getSelectedId() - 1));
            if (newTarget == route_.targetIndex)
                return;
            if (route_.isActive())
                impl_.processor_.removeModRouteLive(route_.source, route_.destination, route_.targetIndex);
            route_.targetIndex = newTarget;
            if (enabled_)
                impl_.processor_.setOrReplaceModRouteLive(route_.source, route_.destination, route_.targetIndex,
                                                            route_.amount, route_.scope);
            impl_.rebuildMatrixRows();
        }

        void commitAmountChange()
        {
            route_.amount = static_cast<float>(amountSlider_.getValue());
            amountLabel_.setText(formatAmountDisplay(route_.destination, route_.amount), juce::dontSendNotification);
            if (enabled_ && route_.isActive())
                updateModRouteAmount(impl_.processor_, route_, route_.amount);
        }

        Impl& impl_;
        modulation::ModRoute route_;
        bool enabled_ = true;
        juce::ToggleButton onToggle_{"On"};
        juce::ComboBox sourceCombo_;
        juce::ComboBox destCombo_;
        juce::ComboBox targetCombo_;
        juce::Slider amountSlider_;
        juce::Label amountLabel_;
    };

    struct ModMatrixScreen::Impl::EnvModuleCard : public juce::Component, private juce::Timer
    {
        EnvModuleCard(PatchworkEightProcessor& processor, std::size_t envIndex)
            : processor_(processor), envIndex_(envIndex), curveView_(processor.apvts, envIndex)
        {
            titleLabel_.setFont(fonts::label(10.0f));
            titleLabel_.setColour(juce::Label::textColourId, palette::kAccentWarm);
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
            g.setColour(palette::kBorder.withAlpha(0.55f));
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 4.0f, 1.0f);
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
            titleLabel_.setColour(juce::Label::textColourId, palette::kAccentWarm);
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
            g.setColour(palette::kBorder.withAlpha(0.55f));
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 4.0f, 1.0f);
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
            auto row = std::make_unique<MatrixRow>(*this, route, enabled);
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
            btn->setColour(juce::TextButton::textColourOffId, palette::kAccentWarm);
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
        auto onCol = header.removeFromLeft(28);
        g.drawText("On", onCol, juce::Justification::centred);
        header.removeFromLeft(4);
        g.setColour(palette::kAccentWarm.withAlpha(0.85f));
        g.drawText("SOURCE", header.removeFromLeft(118), juce::Justification::centredLeft);
        header.removeFromLeft(4);
        g.drawText("TARGET", header.removeFromLeft(148), juce::Justification::centredLeft);
        header.removeFromLeft(58);
        g.setColour(palette::kTextDim);
        g.drawText("AMOUNT", header, juce::Justification::centredLeft);
    }

    void ModMatrixScreen::timerCallback()
    {
        for (auto& row : impl_->matrixRows_)
            row->syncFromRoute();
    }

} // namespace pw8::plugin::ui
