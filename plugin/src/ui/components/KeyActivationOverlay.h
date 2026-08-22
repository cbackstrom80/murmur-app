#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "content/LicenseStore.h"
#include "net/MurmurApiClient.h"

namespace pw8::plugin::ui
{
    /// Real license/API-key activation screen — matches the Figma
    /// `murmur-vst-key-activation` frame (kelp background, whale mark,
    /// "ACTIVATE YOUR ENGINE" panel). Calls murmur-web's real
    /// /api/v1/activate over the network (net::MurmurApiClient), persists
    /// the result via content::LicenseStore, then fetches a starter patch
    /// library from /api/v1/library on success.
    ///
    /// Shown once after the splash if no license is stored locally. Not a
    /// hard gate: SKIP FOR NOW dismisses it with no consequence, since
    /// nothing in the plugin actually enforces entitlement yet (Stripe
    /// isn't wired on the backend) -- a hard lock here would fabricate a
    /// restriction that doesn't functionally exist.
    class KeyActivationOverlay : public juce::Component
    {
    public:
        /// Real per-product branding seam, same shape as SplashOverlay's own
        /// (see that class) -- Undertow's real approved
        /// `undertow-vst-key-activation` Figma frame (node `282:4`) instead
        /// of MURMUR's `murmur-vst-key-activation` one. `subtitle`/
        /// `description` empty means "don't draw this line" -- MURMUR's real
        /// current screen doesn't show either, so its default stays exactly
        /// as before.
        struct Branding
        {
            juce::Image markIcon;
            juce::Image backgroundImage;
            juce::Colour accentColour;
            juce::String subtitle;    // under the mark, e.g. "BASS-FOCUSED COGNITIVE SUB-SYNTHESIZER"
            juce::String panelTitle;  // "ACTIVATE YOUR ENGINE" / "ACTIVATE YOUR LICENSE"
            juce::String description; // real copy under the title explaining what unlocks
            juce::String getKeyLinkText;
            juce::String footerLeft;

            /// Real MURMUR defaults -- byte-identical visuals to before this
            /// Branding seam existed.
            static Branding makeMurmurDefault();
        };

        KeyActivationOverlay();
        explicit KeyActivationOverlay(Branding branding);
        ~KeyActivationOverlay() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        /// Called once the overlay should be removed from view -- either
        /// activation succeeded (and the library fetch, if any, has been
        /// kicked off) or the user chose SKIP FOR NOW.
        std::function<void()> onDismissed;

        /// Fires after a successful activation + library fetch, with the
        /// real patches murmur-web returned, so the parent editor can offer
        /// to install them. Empty on skip or on a library-fetch failure
        /// (activation itself still counts as a success in that case --
        /// the key is real and stored, only the sync failed).
        std::function<void(const juce::Array<net::LibraryPatch>&)> onLibraryFetched;

    private:
        enum class Status
        {
            Idle,
            Activating,
            FetchingLibrary,
            Success,
            Error
        };

        void attemptActivate();
        void setStatus(Status status, const juce::String& message);
        void dismiss();
        [[nodiscard]] juce::Rectangle<float> panelBounds() const;

        Branding branding_;

        juce::Label subtitleLabel_;
        juce::Label panelTitleLabel_;
        juce::Label descriptionLabel_;
        juce::TextEditor keyInput_;
        juce::Label statusLabel_;
        juce::TextButton activateButton_{"ACTIVATE LICENSE"};
        juce::HyperlinkButton getKeyLink_;
        juce::HyperlinkButton skipLink_;

        content::LicenseStore licenseStore_;
        Status status_ = Status::Idle;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyActivationOverlay)
    };

} // namespace pw8::plugin::ui
