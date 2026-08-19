#include "UpdateBadge.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "net/UpdateChecker.h"

namespace pw8::plugin::ui
{
    UpdateBadge::UpdateBadge()
    {
        link_.setFont(fonts::micro(10.0f), false);
        link_.setColour(juce::HyperlinkButton::textColourId, palette::kFigmaBgDeep);
        link_.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(link_);
        setVisible(false); // stays hidden until checkNow() confirms a real update
    }

    UpdateBadge::~UpdateBadge() = default;

    void UpdateBadge::checkNow()
    {
        net::UpdateChecker::checkForUpdate([this](net::UpdateCheckResult result) {
            if (!result.success || !result.updateAvailable)
                return; // no nag on a failed check or when already current

            latestVersion_ = result.latestVersion;
            // ASCII dash, not an em-dash -- this codebase's fonts (Avenir Next
            // at small sizes) garble certain UTF-8 punctuation, see
            // ObsidianFonts.h's kDash/kArrow/kSep and their doc comment.
            link_.setButtonText(juce::String("UPDATE AVAILABLE") + fonts::kDash + "v" + latestVersion_);
            link_.setURL(juce::URL(result.releaseUrl));
            resized();
            setVisible(true);
        });
    }

    void UpdateBadge::paint(juce::Graphics& g)
    {
        if (!isVisible())
            return;

        g.setColour(palette::kAccent);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
    }

    void UpdateBadge::resized()
    {
        link_.setBounds(getLocalBounds());
    }

} // namespace pw8::plugin::ui
