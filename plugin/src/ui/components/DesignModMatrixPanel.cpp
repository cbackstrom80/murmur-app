#include "DesignModMatrixPanel.h"

#include "../PerformanceMetricsUi.h"

#include "../PlayModeLayout.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ModRoutingUi.h"
#include "pw8/modulation/ModCurveShaping.hpp"
#include "pw8/core/Types.hpp"

namespace pw8::plugin::ui
{
    namespace
    {
        struct SourceRowSpec
        {
            const char* label;
            modulation::ModSource source;
            bool interactive;
        };

        struct DestColSpec
        {
            const char* label;
            modulation::ModDestination destination;
            std::uint8_t targetIndex;
        };

        constexpr std::array<SourceRowSpec, layout::kDesignModMatrixPageSourceRowCount> kSourceRows{{
            {"LFO 1", modulation::ModSource::Lfo1, true},
            {"LFO 2", modulation::ModSource::Lfo2, true},
            {"LFO 3", modulation::ModSource::Lfo3, true},
            {"LFO 4", modulation::ModSource::Lfo4, true},
            {"ENV 1", modulation::ModSource::Env1, true},
            {"ENV 2", modulation::ModSource::Env2, true},
            {"ENV 3", modulation::ModSource::Env3, true},
            {"VELOCITY", modulation::ModSource::Velocity, true},
            {"KEY TRACK", modulation::ModSource::None, false},
            {"MOD WHEEL", modulation::ModSource::ModWheel, true},
            {"RANDOM", modulation::ModSource::RandomT, true},
        }};

        constexpr std::array<DestColSpec, layout::kDesignModMatrixPageDestColumnCount> kDestCols{{
            {"OSC PITCH", modulation::ModDestination::OperatorLevel, 0},
            {"FILTER CUT", modulation::ModDestination::FilterCutoff, 0},
            {"FILTER RES", modulation::ModDestination::FilterResonance, 0},
            {"FILT MORPH", modulation::ModDestination::FilterModeMorph, 0},
            {"FILT ROUTE", modulation::ModDestination::FilterRouting, 0},
            {"F2 DRIVE", modulation::ModDestination::FilterDrive, 0},
            {"PAN", modulation::ModDestination::Pan, 0},
            {"MORPH POS", modulation::ModDestination::MorphPosition, 0},
        }};

        struct SidebarPillSpec
        {
            modulation::ModSource source;
            const char* label;
            juce::Colour colour;
            bool interactive;
        };

        const std::array<SidebarPillSpec, 14> kSidebarPills{{
            {modulation::ModSource::Lfo1, "LFO1", palette::kModLfo, true},
            {modulation::ModSource::Lfo2, "LFO2", palette::kModLfo, true},
            {modulation::ModSource::Env1, "ENV1", palette::kModEnv, true},
            {modulation::ModSource::Env2, "ENV2", palette::kModEnv, true},
            {modulation::ModSource::Velocity, "VEL", palette::kModVelocity, true},
            {modulation::ModSource::None, "KEY", palette::kTextDim, false},
            {modulation::ModSource::ModWheel, "MOD", palette::kModModWheel, true},
            {modulation::ModSource::RandomT, "T", palette::kMurmurViolet, true},
            {modulation::ModSource::RandomX, "X", palette::kMurmurViolet, true},
            {modulation::ModSource::Random1, "R1", palette::kMurmurViolet, true},
            {modulation::ModSource::Random2, "R2", palette::kMurmurViolet, true},
            {modulation::ModSource::Random3, "R3", palette::kMurmurViolet, true},
            {modulation::ModSource::Random4, "R4", palette::kMurmurViolet, true},
            {modulation::ModSource::None, "RND", palette::kTextDim, false},
        }};
    } // namespace

    DesignModMatrixPanel::DesignModMatrixPanel(PatchworkEightProcessor& processor,
                                               ModAssignmentController& assignmentController)
        : processor_(processor), assignmentController_(assignmentController)
    {
        backButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        backButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        backButton_.onClick = [this] { dismiss(); };
        addAndMakeVisible(backButton_);

        routesViewport_.setViewedComponent(&routesContent_, false);
        routesViewport_.setScrollBarsShown(true, false);
        addAndMakeVisible(routesViewport_);

        for (const auto& pill : kSidebarPills)
        {
            if (!pill.interactive)
                continue;
            auto chip = std::make_unique<ModSourceChip>(assignmentController_, pill.source, pill.label, pill.colour);
            addAndMakeVisible(*chip);
            sidebarSourceChips_.push_back(std::move(chip));
        }

        startTimerHz(8);
    }

    void DesignModMatrixPanel::showOverlay()
    {
        setVisible(true);
        setInterceptsMouseClicks(true, true);
        resized();
    }

    void DesignModMatrixPanel::dismiss()
    {
        setVisible(false);
        if (onClosed)
            onClosed();
    }

    void DesignModMatrixPanel::setShell(Shell shell)
    {
        if (shell_ == shell)
            return;

        shell_ = shell;
        backButton_.setVisible(shell_ == Shell::DesignPage);
        backButton_.setButtonText("← ENGINE");
        resized();
        repaint();
    }

    void DesignModMatrixPanel::setEmbeddedInDesignMode(bool embedded)
    {
        setShell(embedded ? Shell::DesignPage : Shell::Inline);
    }

    void DesignModMatrixPanel::repaintModAssignmentState()
    {
        for (auto& chip : sidebarSourceChips_)
            chip->repaint();
        repaint();
    }

    void DesignModMatrixPanel::timerCallback()
    {
        pollMidiLearn();
        repaint();
    }

    std::optional<modulation::ModRoute> DesignModMatrixPanel::findRoute(modulation::ModSource source,
                                                                        modulation::ModDestination destination,
                                                                        std::uint8_t targetIndex) const
    {
        for (const auto& route : processor_.getCurrentPatch().layerA.modRoutes)
        {
            if (route.isActive() && route.source == source && route.destination == destination &&
                route.targetIndex == targetIndex)
                return route;
        }
        return std::nullopt;
    }

    float DesignModMatrixPanel::routeDepthNormalized(const modulation::ModRoute& route) const
    {
        const auto range = modAmountRangeFor(route.destination);
        return juce::jlimit(0.0f, 1.0f, (route.amount - range.min) / juce::jmax(0.001f, range.max - range.min));
    }

    const char* DesignModMatrixPanel::curveLabelForRoute(const modulation::ModRoute& route) const
    {
        return modulation::modCurveLabel(route.curve);
    }

    void DesignModMatrixPanel::selectRouteForLearn(const modulation::ModRoute& route)
    {
        selectedLearnRoute_ = route;
        repaint();
    }

    void DesignModMatrixPanel::toggleRoutePolar(const modulation::ModRoute& route)
    {
        const float magnitude = juce::jmax(0.001f, std::abs(route.amount));
        updateModRouteAmount(processor_, route, route.amount < 0.0f ? magnitude : -magnitude);
        selectRouteForLearn(route);
        repaint();
    }

    void DesignModMatrixPanel::cycleRouteCurve(const modulation::ModRoute& route)
    {
        const auto next = modulation::cycleModCurve(route.curve);
        updateModRouteCurve(processor_, route, next);
        modulation::ModRoute updated = route;
        updated.curve = next;
        selectRouteForLearn(updated);
        repaint();
    }

    void DesignModMatrixPanel::pollMidiLearn()
    {
        if (!midiLearnActive_)
            return;

        int cc = -1;
        int value7 = 0;
        if (!processor_.consumeMidiLearnCc(cc, value7))
            return;

        lastLearnCc_ = cc;

        if (!selectedLearnRoute_.has_value())
            return;

        const auto range = modAmountRangeFor(selectedLearnRoute_->destination);
        const float norm = static_cast<float>(value7) / 127.0f;
        const float amount = range.min + norm * (range.max - range.min);
        updateModRouteAmount(processor_, *selectedLearnRoute_, amount);

        for (const auto& route : processor_.getCurrentPatch().layerA.modRoutes)
        {
            if (route.isActive() && route.source == selectedLearnRoute_->source
                && route.destination == selectedLearnRoute_->destination
                && route.targetIndex == selectedLearnRoute_->targetIndex)
            {
                selectedLearnRoute_ = route;
                break;
            }
        }

        repaint();
    }

    juce::Rectangle<int> DesignModMatrixPanel::gridCanvasBounds() const
    {
        auto canvas = gridCardBounds_.reduced(layout::kDesignModMatrixPageCardPadding);
        canvas.removeFromTop(layout::kDesignModMatrixPageCardHeaderHeight + layout::kDesignModMatrixPageCardInnerGap);
        return canvas.reduced(layout::kDesignModMatrixPageGridCanvasPadding);
    }

    std::optional<std::pair<int, int>> DesignModMatrixPanel::gridCellAt(juce::Point<int> pos) const
    {
        const auto canvas = gridCanvasBounds();
        if (!canvas.contains(pos))
            return std::nullopt;

        auto local = pos - canvas.getTopLeft();
        local.y -= layout::kDesignModMatrixPageGridHeaderHeight + layout::kDesignModMatrixPageGridColumnGap;
        if (local.y < 0)
            return std::nullopt;

        const int row = local.y / layout::kDesignModMatrixPageGridRowHeight;
        if (row < 0 || row >= layout::kDesignModMatrixPageSourceRowCount)
            return std::nullopt;

        local.x -= layout::kDesignModMatrixPageSourceColumnWidth + layout::kDesignModMatrixPageGridColumnGap;
        if (local.x < 0)
            return std::nullopt;

        const int colWidth = layout::kDesignModMatrixPageDestColumnWidth + layout::kDesignModMatrixPageGridColumnGap;
        const int col = local.x / colWidth;
        if (col < 0 || col >= layout::kDesignModMatrixPageDestColumnCount)
            return std::nullopt;

        return std::make_pair(row, col);
    }

    std::vector<DesignModMatrixPanel::RouteRowHit> DesignModMatrixPanel::layoutRouteRows() const
    {
        std::vector<RouteRowHit> hits;
        auto area = routesContent_.getLocalBounds().reduced(layout::kDesignModMatrixPageCardPadding);
        area.removeFromTop(layout::kDesignModMatrixPageCardHeaderHeight + layout::kDesignModMatrixPageCardInnerGap);

        for (const auto& route : processor_.getCurrentPatch().layerA.modRoutes)
        {
            if (!route.isActive())
                continue;

            auto row = area.removeFromTop(layout::kDesignModMatrixPageRouteRowHeight);
            area.removeFromTop(layout::kDesignModMatrixPageRouteRowGap);
            const auto rowArea = row;

            row.removeFromLeft(layout::kDesignModMatrixPageRouteSourceWidth + 12
                               + layout::kDesignModMatrixPageRouteDestWidth + 12);
            const auto depthArea = row.removeFromLeft(layout::kDesignModMatrixPageRouteDepthTrackWidth + 66);

            auto tail = row;
            const int polarW = route.amount < 0.0f ? 27 : 17;
            const auto polarArea = tail.removeFromRight(polarW).reduced(0, 6);
            tail.removeFromRight(4);
            const auto curveArea = tail.removeFromRight(27).reduced(0, 6);

            hits.push_back({route, rowArea, depthArea, curveArea, polarArea});
        }
        return hits;
    }

    void DesignModMatrixPanel::paintCardChrome(juce::Graphics& g, juce::Rectangle<int> bounds,
                                               const juce::String& title, juce::Colour ledColour,
                                               const juce::String& subtitle) const
    {
        g.setColour(palette::kPanelRaised.withAlpha(0.55f));
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 8.0f, 1.0f);

        auto header = bounds.reduced(layout::kDesignModMatrixPageCardPadding)
                          .removeFromTop(layout::kDesignModMatrixPageCardHeaderHeight);
        g.setColour(ledColour);
        g.fillEllipse(static_cast<float>(header.getX()), static_cast<float>(header.getCentreY() - 3), 6.0f, 6.0f);

        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::label(11.0f));
        g.drawText(title, header.withTrimmedLeft(14), juce::Justification::centredLeft);

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText(subtitle, header, juce::Justification::centredRight);
    }

    void DesignModMatrixPanel::paintCellDial(juce::Graphics& g, juce::Rectangle<float> bounds,
                                             float normalizedDepth) const
    {
        const float dialSize = static_cast<float>(layout::kDesignModMatrixPageCellDialSize);
        auto dial = bounds.withSizeKeepingCentre(dialSize, dialSize);

        g.setColour(palette::kPanel);
        g.fillEllipse(dial);

        if (normalizedDepth <= 0.001f)
        {
            g.setColour(palette::kTextDim.withAlpha(0.45f));
            g.fillEllipse(dial.withSizeKeepingCentre(static_cast<float>(layout::kDesignModMatrixPageCellEmptySize),
                                                     static_cast<float>(layout::kDesignModMatrixPageCellEmptySize)));
            return;
        }

        juce::Path arc;
        const float start = juce::MathConstants<float>::pi * 1.25f;
        const float sweep = juce::MathConstants<float>::twoPi * normalizedDepth;
        arc.addCentredArc(dial.getCentreX(), dial.getCentreY(), dial.getWidth() * 0.5f - 1.5f,
                          dial.getHeight() * 0.5f - 1.5f, 0.0f, start, start + sweep, true);
        g.setColour(palette::kAccent);
        g.strokePath(arc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour(palette::kAccent.withAlpha(0.85f));
        g.fillEllipse(dial.withSizeKeepingCentre(4.0f, 4.0f));
    }

    void DesignModMatrixPanel::paintInterconnectGrid(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        paintCardChrome(g, bounds, "MODULATION INTERCONNECT GRID", palette::kAccent,
                        "ACTIVE ROUTINGS INDICATED IN TEAL");

        auto canvas = bounds.reduced(layout::kDesignModMatrixPageCardPadding);
        canvas.removeFromTop(layout::kDesignModMatrixPageCardHeaderHeight + layout::kDesignModMatrixPageCardInnerGap);
        canvas = canvas.reduced(layout::kDesignModMatrixPageGridCanvasPadding);

        g.setColour(palette::kBackgroundTop);
        g.fillRoundedRectangle(canvas.toFloat(), 6.0f);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawRoundedRectangle(canvas.toFloat().reduced(0.5f), 6.0f, 1.0f);

        auto headerRow = canvas.removeFromTop(layout::kDesignModMatrixPageGridHeaderHeight);
        headerRow.removeFromBottom(layout::kDesignModMatrixPageGridColumnGap);

        g.setFont(fonts::label(8.0f));
        g.setColour(palette::kTextDim);
        g.drawText("SOURCE", headerRow.removeFromLeft(layout::kDesignModMatrixPageSourceColumnWidth),
                   juce::Justification::centredLeft);
        headerRow.removeFromLeft(layout::kDesignModMatrixPageGridColumnGap);

        for (const auto& dest : kDestCols)
        {
            g.setColour(palette::kTextSecondary);
            g.setFont(fonts::label(7.0f));
            g.drawText(dest.label, headerRow.removeFromLeft(layout::kDesignModMatrixPageDestColumnWidth),
                       juce::Justification::centred);
            headerRow.removeFromLeft(layout::kDesignModMatrixPageGridColumnGap);
        }

        for (int row = 0; row < layout::kDesignModMatrixPageSourceRowCount; ++row)
        {
            auto rowBounds = canvas.removeFromTop(layout::kDesignModMatrixPageGridRowHeight);
            if (row % 2 == 0)
            {
                g.setColour(palette::kPanelRaised.withAlpha(0.35f));
                g.fillRoundedRectangle(rowBounds.toFloat().reduced(0.0f, 0.5f), 3.0f);
            }

            const auto& source = kSourceRows[static_cast<std::size_t>(row)];
            g.setColour(source.interactive ? palette::kTextPrimary : palette::kTextDim);
            g.setFont(fonts::label(8.0f));
            g.drawText(source.label, rowBounds.removeFromLeft(layout::kDesignModMatrixPageSourceColumnWidth),
                       juce::Justification::centredLeft);
            rowBounds.removeFromLeft(layout::kDesignModMatrixPageGridColumnGap);

            for (int col = 0; col < layout::kDesignModMatrixPageDestColumnCount; ++col)
            {
                const auto& dest = kDestCols[static_cast<std::size_t>(col)];
                auto cell = rowBounds.removeFromLeft(layout::kDesignModMatrixPageDestColumnWidth);

                float depth = 0.0f;
                if (source.interactive)
                {
                    if (const auto route = findRoute(source.source, dest.destination, dest.targetIndex))
                        depth = routeDepthNormalized(*route);
                }

                paintCellDial(g, cell.toFloat(), depth);
                rowBounds.removeFromLeft(layout::kDesignModMatrixPageGridColumnGap);
            }
        }
    }

    void DesignModMatrixPanel::paintActiveRoutes(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        paintCardChrome(g, bounds, "ACTIVE ROUTING PATHS", palette::kAccentWarm,
                        "BIPOLAR SCALING OPTIONS DETECTED");

        auto table = bounds.reduced(layout::kDesignModMatrixPageCardPadding);
        table.removeFromTop(layout::kDesignModMatrixPageCardHeaderHeight + layout::kDesignModMatrixPageCardInnerGap);

        g.saveState();
        g.reduceClipRegion(table);
        auto area = routesContent_.getLocalBounds().reduced(layout::kDesignModMatrixPageCardPadding);
        area.removeFromTop(layout::kDesignModMatrixPageCardHeaderHeight + layout::kDesignModMatrixPageCardInnerGap);

        int rowIndex = 0;
        for (const auto& route : processor_.getCurrentPatch().layerA.modRoutes)
        {
            if (!route.isActive())
                continue;

            auto row = area.removeFromTop(layout::kDesignModMatrixPageRouteRowHeight);
            area.removeFromTop(layout::kDesignModMatrixPageRouteRowGap);

            if (rowIndex % 2 == 1)
            {
                g.setColour(palette::kPanelRaised.withAlpha(0.25f));
                g.fillRoundedRectangle(row.toFloat(), 3.0f);
            }

            const bool selectedLearn =
                selectedLearnRoute_.has_value() && route.source == selectedLearnRoute_->source
                && route.destination == selectedLearnRoute_->destination
                && route.targetIndex == selectedLearnRoute_->targetIndex;
            if (selectedLearn)
            {
                g.setColour(palette::kAccent.withAlpha(0.12f));
                g.fillRoundedRectangle(row.toFloat(), 3.0f);
            }

            g.setColour(palette::modSourceColour(static_cast<int>(route.source)));
            g.setFont(fonts::label(8.0f));
            g.drawText(modSourceLabel(route.source), row.removeFromLeft(layout::kDesignModMatrixPageRouteSourceWidth),
                       juce::Justification::centredLeft);

            g.setColour(palette::kTextDim);
            g.drawText(fonts::kArrow, row.removeFromLeft(12), juce::Justification::centred);

            g.setColour(palette::kTextPrimary);
            g.drawText(modDestinationLabel(route.destination, route.targetIndex),
                       row.removeFromLeft(layout::kDesignModMatrixPageRouteDestWidth), juce::Justification::centredLeft);

            auto sliderGroup = row;
            g.setColour(palette::kTextDim);
            g.setFont(fonts::label(8.0f));
            g.drawText("DEPTH", sliderGroup.removeFromLeft(24), juce::Justification::centredLeft);
            sliderGroup.removeFromLeft(6);

            const float norm = routeDepthNormalized(route);
            auto track = sliderGroup.removeFromLeft(layout::kDesignModMatrixPageRouteDepthTrackWidth).reduced(0, 2);
            g.setColour(palette::kPanel);
            g.fillRoundedRectangle(track.toFloat(), 3.0f);
            g.setColour(palette::kAccent);
            g.fillRoundedRectangle(track.getX() + 1, track.getY() + 1,
                                   static_cast<int>((track.getWidth() - 2) * norm), track.getHeight() - 2, 2.0f);

            g.setColour(palette::kTextPrimary);
            const int pct = juce::roundToInt(norm * 100.0f);
            g.drawText(juce::String(pct) + "%", sliderGroup.removeFromLeft(36), juce::Justification::centredLeft);

            auto curveBox = row.removeFromRight(27).reduced(0, 6);
            g.setColour(palette::kPanel);
            g.fillRoundedRectangle(curveBox.toFloat(), 3.0f);
            g.setColour(palette::kTextDim);
            g.drawText(curveLabelForRoute(route), curveBox, juce::Justification::centred);

            row.removeFromRight(4);
            auto polarBox = row.removeFromRight(route.amount < 0.0f ? 27 : 17).reduced(0, 6);
            g.setColour(route.amount < 0.0f ? palette::kAccent.withAlpha(0.15f) : palette::kPanel);
            g.fillRoundedRectangle(polarBox.toFloat(), 3.0f);
            g.setColour(route.amount < 0.0f ? palette::kAccent : palette::kTextDim);
            g.drawRoundedRectangle(polarBox.toFloat(), 3.0f, 0.8f);
            g.drawText(route.amount < 0.0f ? "+/-" : "+", polarBox, juce::Justification::centred);

            ++rowIndex;
        }

        if (rowIndex == 0)
        {
            g.setColour(palette::kTextDim);
            g.setFont(fonts::value(9.0f));
            g.drawFittedText("No active routes. Click a grid cell or sidebar source pill to assign.",
                             table, juce::Justification::centred, 2);
        }

        g.restoreState();
    }

    void DesignModMatrixPanel::paintQuickConfigSidebar(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        g.setColour(palette::kPanelRaised.withAlpha(0.35f));
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 8.0f, 1.0f);

        auto inner = bounds.reduced(layout::kDesignModMatrixPageSidebarPadding);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText("QUICK CONFIG", inner.removeFromTop(10), juce::Justification::centredLeft);
        inner.removeFromTop(12);

        auto dragCard = inner.removeFromTop(76);
        g.setColour(palette::kBackgroundTop.withAlpha(0.65f));
        g.fillRoundedRectangle(dragCard.toFloat(), 6.0f);
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawRoundedRectangle(dragCard.toFloat().reduced(0.5f), 6.0f, 1.0f);
        auto dragInner = dragCard.reduced(10);
        g.setColour(palette::kTextDim);
        g.drawText("DRAG MOD SOURCE", dragInner.removeFromTop(10), juce::Justification::centredLeft);
        dragInner.removeFromTop(6);

        for (const auto& pill : kSidebarPills)
        {
            if (pill.interactive)
                continue;

            const bool isRnd = pill.label[0] == 'R';
            const int x = isRnd ? dragInner.getX() : dragInner.getX() + 175;
            const int y = isRnd ? dragInner.getY() + 22 : dragInner.getY();
            auto pillBounds =
                juce::Rectangle<int>(x, y, 27, layout::kDesignModMatrixPageQuickConfigPillHeight);
            g.setColour(palette::kPanelRaised.withAlpha(0.5f));
            g.fillRoundedRectangle(pillBounds.toFloat(), 4.0f);
            g.setColour(palette::kTextDim);
            g.drawText(pill.label, pillBounds, juce::Justification::centred);
        }

        inner.removeFromTop(12);
        auto randomCard = inner.removeFromTop(92);
        g.setColour(palette::kBackgroundTop.withAlpha(0.65f));
        g.fillRoundedRectangle(randomCard.toFloat(), 6.0f);
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawRoundedRectangle(randomCard.toFloat().reduced(0.5f), 6.0f, 1.0f);
        auto randomInner = randomCard.reduced(10);
        g.setColour(palette::kTextDim);
        g.drawText("RANDOM SOURCES", randomInner.removeFromTop(10), juce::Justification::centredLeft);
        randomInner.removeFromTop(6);
        g.drawText("DEJA-VU", randomInner.removeFromTop(10), juce::Justification::centredLeft);
        randomInner.removeFromTop(4);
        g.setColour(palette::kMurmurViolet.withAlpha(0.15f));
        g.fillRoundedRectangle(randomInner.removeFromTop(18).toFloat(), 4.0f);
        randomInner.removeFromTop(6);
        g.setColour(palette::kTextDim);
        g.drawText("T / X CLOCKS", randomInner.removeFromTop(10), juce::Justification::centredLeft);

        inner.removeFromTop(12);
        auto recentCard = inner.removeFromTop(106);
        g.setColour(palette::kBackgroundTop.withAlpha(0.65f));
        g.fillRoundedRectangle(recentCard.toFloat(), 6.0f);
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawRoundedRectangle(recentCard.toFloat().reduced(0.5f), 6.0f, 1.0f);
        auto recentInner = recentCard.reduced(10);
        g.setColour(palette::kTextDim);
        g.drawText("RECENT DESTINATIONS", recentInner.removeFromTop(10), juce::Justification::centredLeft);
        recentInner.removeFromTop(8);

        juce::StringArray recent;
        for (const auto& route : processor_.getCurrentPatch().layerA.modRoutes)
        {
            if (!route.isActive())
                continue;
            const auto label = modDestinationLabel(route.destination, route.targetIndex);
            if (!recent.contains(label))
                recent.add(label);
            if (recent.size() >= 3)
                break;
        }
        if (recent.isEmpty())
        {
            recent.add("OSC 01 FILTER RES");
            recent.add("OSC 02 PAN");
            recent.add("GLOBAL REVERB TIME");
        }

        int destY = recentInner.getY();
        for (const auto& dest : recent)
        {
            auto chip = juce::Rectangle<int>(recentInner.getX(), destY, juce::jmin(recentInner.getWidth(), 120), 20);
            g.setColour(palette::kPanel);
            g.fillRoundedRectangle(chip.toFloat(), 4.0f);
            g.setColour(palette::kTextSecondary);
            g.setFont(fonts::label(8.0f));
            g.drawText(dest, chip.reduced(8, 5), juce::Justification::centredLeft);
            destY += 24;
        }

        inner.removeFromTop(12);
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawHorizontalLine(inner.getY(), static_cast<float>(inner.getX()),
                             static_cast<float>(inner.getRight()));
        inner.removeFromTop(12);

        auto learnCard = inner.removeFromTop(111);
        g.setColour(palette::kBackgroundTop.withAlpha(0.65f));
        g.fillRoundedRectangle(learnCard.toFloat(), 6.0f);
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawRoundedRectangle(learnCard.toFloat().reduced(0.5f), 6.0f, 1.0f);
        auto learnInner = learnCard.reduced(12);
        g.setColour(palette::kTextDim);
        g.drawText("MIDI INTERFACE MAPPING", learnInner.removeFromTop(10), juce::Justification::centredLeft);
        learnInner.removeFromTop(10);
        auto learnBtn = learnInner.removeFromTop(37);
        midiLearnButtonBounds_ = learnBtn;
        g.setColour(midiLearnActive_ ? palette::kAccent.withAlpha(0.22f) : palette::kAccent.withAlpha(0.12f));
        g.fillRoundedRectangle(learnBtn.toFloat(), 6.0f);
        g.setColour(palette::kAccent);
        g.drawRoundedRectangle(learnBtn.toFloat(), 6.0f, 1.0f);
        g.fillEllipse(static_cast<float>(learnBtn.getCentreX() - 52), static_cast<float>(learnBtn.getCentreY() - 4), 8.0f,
                      8.0f);
        g.setColour(palette::kAccent);
        g.drawText(midiLearnActive_ ? "MIDI LEARN ACTIVE" : "START MIDI LEARN", learnBtn.withTrimmedLeft(24),
                   juce::Justification::centredLeft);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::value(8.0f));
        juce::String learnHint =
            "Twist any physical dial on your MIDI controller to bind to the selected parameter.";
        if (selectedLearnRoute_.has_value())
            learnHint = "Selected: " + modSourceLabel(selectedLearnRoute_->source) + " " + fonts::kArrow + " "
                        + modDestinationLabel(selectedLearnRoute_->destination, selectedLearnRoute_->targetIndex)
                        + (lastLearnCc_ >= 0 ? juce::String(fonts::kSep) + "CC " + juce::String(lastLearnCc_)
                                               : juce::String{});
        g.drawFittedText(learnHint, learnInner, juce::Justification::topLeft, 3);

        inner.removeFromTop(12);
        auto info = inner.removeFromTop(46);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::value(8.0f));
        g.drawFittedText("* BIPOLAR (+/-) expands depth modulation symmetrically around current absolute offset value.",
                         info.reduced(8), juce::Justification::topLeft, 3);
    }

    void DesignModMatrixPanel::paintStatusBar(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        g.setColour(palette::kPanelRaised.withAlpha(0.92f));
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 8.0f, 1.0f);

        auto row = bounds.reduced(16, 15);
        const auto metrics = readPerformanceMetrics(processor_);
        g.setFont(fonts::label(8.0f));
        g.setColour(palette::kTextDim);
        g.drawText("CPU", row.removeFromLeft(15), juce::Justification::centredLeft);
        row.removeFromLeft(6);
        auto cpuTrack = row.removeFromLeft(40).reduced(0, 3);
        g.setColour(palette::kPanel);
        g.fillRoundedRectangle(cpuTrack.toFloat(), 2.0f);
        g.setColour(palette::kAccent);
        g.fillRoundedRectangle(cpuTrack.getX(), cpuTrack.getY(),
                               static_cast<int>(cpuTrack.getWidth() * cpuBarFillRatio(metrics.cpuPercent)),
                               cpuTrack.getHeight(), 2.0f);
        g.setColour(palette::kTextDim);
        g.drawText(formatCpuPercent(metrics.cpuPercent), row.removeFromLeft(20), juce::Justification::centredLeft);
        row.removeFromLeft(16);

        g.drawText("VOICES:", row.removeFromLeft(34), juce::Justification::centredLeft);
        g.drawText(formatVoiceCount(metrics.activeVoices, metrics.maxVoices), row.removeFromLeft(40),
                   juce::Justification::centredLeft);
        row.removeFromLeft(16);
        g.setColour(palette::kAccent);
        g.fillEllipse(static_cast<float>(row.getX()), static_cast<float>(row.getY() + 2), 6.0f, 6.0f);
        g.setColour(palette::kTextDim);
        g.drawText("MIDI IN", row.removeFromLeft(40).withTrimmedLeft(10), juce::Justification::centredLeft);

        g.drawText("MURMUR DSP ENGINE VST3" + juce::String(fonts::kSep) + "VERSION 1.4.2" + juce::String(fonts::kSep)
                       + "(C) 2026 MURMUR AUDIO",
                   bounds.reduced(16, 15), juce::Justification::centredRight);
    }

    void DesignModMatrixPanel::toggleGridCell(int sourceRow, int destCol)
    {
        const auto& source = kSourceRows[static_cast<std::size_t>(sourceRow)];
        const auto& dest = kDestCols[static_cast<std::size_t>(destCol)];
        if (!source.interactive)
            return;

        if (findRoute(source.source, dest.destination, dest.targetIndex).has_value())
        {
            processor_.removeModRouteLive(source.source, dest.destination, dest.targetIndex);
        }
        else
        {
            assignModRoute(processor_, source.source, dest.destination, dest.targetIndex);
        }
        repaint();
    }

    void DesignModMatrixPanel::beginDepthDrag(const modulation::ModRoute& route, float startX)
    {
        amountDrag_ = AmountDragState{route, route.amount, startX};
    }

    void DesignModMatrixPanel::continueDepthDrag(float currentX)
    {
        if (!amountDrag_.has_value())
            return;

        const auto range = modAmountRangeFor(amountDrag_->route.destination);
        const float span = range.max - range.min;
        const float delta = (currentX - amountDrag_->startX) * (span / 160.0f);
        updateModRouteAmount(processor_, amountDrag_->route, amountDrag_->startAmount + delta);
        for (const auto& r : processor_.getCurrentPatch().layerA.modRoutes)
        {
            if (r.isActive() && r.source == amountDrag_->route.source &&
                r.destination == amountDrag_->route.destination && r.targetIndex == amountDrag_->route.targetIndex)
            {
                amountDrag_->route.amount = r.amount;
                break;
            }
        }
        repaint();
    }

    void DesignModMatrixPanel::endDepthDrag()
    {
        amountDrag_.reset();
    }

    void DesignModMatrixPanel::mouseDown(const juce::MouseEvent& event)
    {
        if (midiLearnButtonBounds_.contains(event.getPosition()))
        {
            midiLearnActive_ = !midiLearnActive_;
            if (!midiLearnActive_)
                lastLearnCc_ = -1;
            repaint();
            return;
        }

        if (const auto cell = gridCellAt(event.getPosition()))
        {
            toggleGridCell(cell->first, cell->second);
            return;
        }

        const auto pos = event.getPosition() - routesViewport_.getBounds().getTopLeft()
                         + routesViewport_.getViewPosition();
        for (const auto& row : layoutRouteRows())
        {
            if (row.curveArea.contains(pos))
            {
                cycleRouteCurve(row.route);
                return;
            }
            if (row.polarArea.contains(pos))
            {
                toggleRoutePolar(row.route);
                return;
            }
            if (row.depthArea.contains(pos))
            {
                selectRouteForLearn(row.route);
                beginDepthDrag(row.route, static_cast<float>(event.position.x));
                return;
            }
            if (row.rowArea.contains(pos))
            {
                selectRouteForLearn(row.route);
                return;
            }
        }
    }

    void DesignModMatrixPanel::mouseDrag(const juce::MouseEvent& event)
    {
        if (amountDrag_.has_value())
            continueDepthDrag(static_cast<float>(event.position.x));
    }

    void DesignModMatrixPanel::mouseUp(const juce::MouseEvent&)
    {
        endDepthDrag();
    }

    void DesignModMatrixPanel::paint(juce::Graphics& g)
    {
        juce::ignoreUnused(g);
        paintQuickConfigSidebar(g, sidebarBounds_);
        paintStatusBar(g, statusBarBounds_);
    }

    bool DesignModMatrixPanel::keyPressed(const juce::KeyPress& key)
    {
        if (shell_ == Shell::DesignPage && key == juce::KeyPress::escapeKey)
        {
            dismiss();
            return true;
        }
        return false;
    }

    void DesignModMatrixPanel::paintOverChildren(juce::Graphics& g)
    {
        paintInterconnectGrid(g, gridCardBounds_);
        paintActiveRoutes(g, routesCardBounds_);
    }

    void DesignModMatrixPanel::resized()
    {
        auto bounds = getLocalBounds();

        if (shell_ == Shell::DesignPage)
        {
            auto header = bounds.removeFromTop(layout::kDesignLabPanelHeaderHeight);
            backButton_.setBounds(header.removeFromLeft(120));
        }

        statusBarBounds_ = bounds.removeFromBottom(layout::kDesignModMatrixPageStatusBarHeight);
        bounds.removeFromBottom(layout::kDesignModMatrixPageSectionGap);

        sidebarBounds_ = bounds.removeFromRight(layout::kDesignModMatrixPageSidebarWidth);
        bounds.removeFromRight(layout::kDesignModMatrixPageSidebarGap);

        gridCardBounds_ = bounds.removeFromTop(layout::kDesignModMatrixPageGridCardHeight);
        bounds.removeFromTop(layout::kDesignModMatrixPageCardGap);
        routesCardBounds_ = bounds;

        routesViewport_.setBounds(routesCardBounds_.reduced(layout::kDesignModMatrixPageCardPadding)
                                      .withTrimmedTop(layout::kDesignModMatrixPageCardHeaderHeight
                                                      + layout::kDesignModMatrixPageCardInnerGap));

        int activeRoutes = 0;
        for (const auto& route : processor_.getCurrentPatch().layerA.modRoutes)
        {
            if (route.isActive())
                ++activeRoutes;
        }
        const int contentHeight =
            juce::jmax(routesViewport_.getHeight(),
                        activeRoutes * (layout::kDesignModMatrixPageRouteRowHeight + layout::kDesignModMatrixPageRouteRowGap));
        routesContent_.setSize(routesViewport_.getWidth(), contentHeight);

        auto sidebarInner = sidebarBounds_.reduced(layout::kDesignModMatrixPageSidebarPadding);
        sidebarInner.removeFromTop(22);
        auto dragCard = sidebarInner.removeFromTop(76).reduced(10);
        dragCard.removeFromTop(16);

        static constexpr struct
        {
            int x;
            int y;
            int w;
        } kChipLayout[] = {
            {0, 0, 32},
            {36, 0, 32},
            {72, 0, 32},
            {108, 0, 32},
            {144, 0, 27},
            {206, 0, 27},
        };

        for (std::size_t i = 0; i < sidebarSourceChips_.size() && i < std::size(kChipLayout); ++i)
        {
            const auto& slot = kChipLayout[i];
            sidebarSourceChips_[i]->setBounds(dragCard.getX() + slot.x, dragCard.getY() + slot.y, slot.w,
                                               layout::kDesignModMatrixPageQuickConfigPillHeight);
        }
    }

} // namespace pw8::plugin::ui
