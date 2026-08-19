#pragma once

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ModAssignmentController.h"
#include "ModSourceChip.h"
#include "processor/PatchworkEightProcessor.h"
#include "pw8/modulation/ModMatrixTypes.hpp"

namespace pw8::plugin::ui
{
    /// Figma `murmur-design-mod-matrix` (27:265) — full-page interconnect grid for design sub-nav.
    class DesignModMatrixPanel : public juce::Component, private juce::Timer
    {
    public:
        DesignModMatrixPanel(PatchworkEightProcessor& processor, ModAssignmentController& assignmentController);

        std::function<void()> onClosed;

        enum class Shell
        {
            /// Embedded inline (no back row).
            Inline,
            /// Design sub-nav MOD page — Figma `27:265`; back returns to ENGINE.
            DesignPage,
        };

        void showOverlay();
        void dismiss();

        void setShell(Shell shell);
        void setEmbeddedInDesignMode(bool embedded);
        void repaintModAssignmentState();

        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        struct RouteRowHit
        {
            modulation::ModRoute route;
            juce::Rectangle<int> rowArea;
            juce::Rectangle<int> depthArea;
            juce::Rectangle<int> curveArea;
            juce::Rectangle<int> polarArea;
        };

        [[nodiscard]] const char* curveLabelForRoute(const modulation::ModRoute& route) const;
        void selectRouteForLearn(const modulation::ModRoute& route);
        void toggleRoutePolar(const modulation::ModRoute& route);
        void cycleRouteCurve(const modulation::ModRoute& route);
        void pollMidiLearn();

        void timerCallback() override;

        [[nodiscard]] std::optional<modulation::ModRoute> findRoute(modulation::ModSource source,
                                                                    modulation::ModDestination destination,
                                                                    std::uint8_t targetIndex) const;
        [[nodiscard]] float routeDepthNormalized(const modulation::ModRoute& route) const;
        [[nodiscard]] juce::Rectangle<int> gridCanvasBounds() const;
        [[nodiscard]] std::optional<std::pair<int, int>> gridCellAt(juce::Point<int> pos) const;
        [[nodiscard]] std::vector<RouteRowHit> layoutRouteRows() const;

        void paintCardChrome(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title,
                             juce::Colour ledColour, const juce::String& subtitle) const;
        void paintInterconnectGrid(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void paintCellDial(juce::Graphics& g, juce::Rectangle<float> bounds, float normalizedDepth) const;
        void paintActiveRoutes(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void paintQuickConfigSidebar(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void paintStatusBar(juce::Graphics& g, juce::Rectangle<int> bounds) const;

        void toggleGridCell(int sourceRow, int destCol);
        void beginDepthDrag(const modulation::ModRoute& route, float startX);
        void continueDepthDrag(float currentX);
        void endDepthDrag();

        PatchworkEightProcessor& processor_;
        ModAssignmentController& assignmentController_;

        Shell shell_ = Shell::Inline;

        juce::TextButton backButton_{"← DESIGN"};
        juce::Viewport routesViewport_;
        juce::Component routesContent_;

        std::vector<std::unique_ptr<ModSourceChip>> sidebarSourceChips_;

        juce::Rectangle<int> gridCardBounds_;
        juce::Rectangle<int> routesCardBounds_;
        juce::Rectangle<int> sidebarBounds_;
        juce::Rectangle<int> statusBarBounds_;

        struct AmountDragState
        {
            modulation::ModRoute route;
            float startAmount = 0.0f;
            float startX = 0.0f;
        };
        std::optional<AmountDragState> amountDrag_;

        std::optional<modulation::ModRoute> selectedLearnRoute_;
        bool midiLearnActive_ = false;
        int lastLearnCc_ = -1;
        mutable juce::Rectangle<int> midiLearnButtonBounds_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DesignModMatrixPanel)
    };

} // namespace pw8::plugin::ui
