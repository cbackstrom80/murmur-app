#include "KeyActivationOverlay.h"

#include "BinaryData.h"
#include "../theme/BrandingAssets.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr float kDangerAlpha = 1.0f;
        const juce::Colour kDanger{0xffff5f2d}; // matches murmur-web's --color-danger token

        /// XXXX-XXXX-XXXX-XXXX, case-insensitive -- matches murmur-web's own
        /// looksLikeLicenseKey() check (src/lib/apikey.ts), kept in sync by
        /// hand since this is a tiny plugin-side pre-check, not the real
        /// validation (the server's hash lookup is that).
        bool looksLikeLicenseKey(const juce::String& text)
        {
            const auto trimmed = text.trim();
            const auto parts = juce::StringArray::fromTokens(trimmed, "-", "");
            if (parts.size() != 4)
                return false;
            for (const auto& part : parts)
                if (part.length() != 4 || !part.containsOnly("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"))
                    return false;
            return true;
        }
    } // namespace

    KeyActivationOverlay::Branding KeyActivationOverlay::Branding::makeMurmurDefault()
    {
        return Branding{
            branding::getWhaleIcon(),
            juce::ImageCache::getFromMemory(BinaryData::murmur_splash_bg_jpg, BinaryData::murmur_splash_bg_jpgSize),
            palette::kAccent,
            "", // no subtitle line under the mark on MURMUR's real current screen
            "ACTIVATE YOUR ENGINE",
            "", // no description line on MURMUR's real current screen
            "DON'T HAVE A KEY?  GET YOUR KEY ->",
            "MURMUR AUDIO (C) 2026",
        };
    }

    KeyActivationOverlay::KeyActivationOverlay() : KeyActivationOverlay(Branding::makeMurmurDefault()) {}

    KeyActivationOverlay::KeyActivationOverlay(Branding branding) : branding_(std::move(branding))
    {
        subtitleLabel_.setText(branding_.subtitle, juce::dontSendNotification);
        subtitleLabel_.setFont(fonts::label(10.0f));
        subtitleLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        subtitleLabel_.setJustificationType(juce::Justification::centred);
        subtitleLabel_.setVisible(branding_.subtitle.isNotEmpty());
        addAndMakeVisible(subtitleLabel_);

        panelTitleLabel_.setText(branding_.panelTitle, juce::dontSendNotification);
        panelTitleLabel_.setFont(fonts::title(15.0f));
        panelTitleLabel_.setColour(juce::Label::textColourId, palette::kFigmaTextPrimary);
        panelTitleLabel_.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(panelTitleLabel_);

        descriptionLabel_.setText(branding_.description, juce::dontSendNotification);
        descriptionLabel_.setFont(fonts::micro(10.0f));
        descriptionLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        descriptionLabel_.setJustificationType(juce::Justification::centred);
        descriptionLabel_.setMinimumHorizontalScale(1.0f);
        descriptionLabel_.setVisible(branding_.description.isNotEmpty());
        addAndMakeVisible(descriptionLabel_);

        keyInput_.setTextToShowWhenEmpty("XXXX-XXXX-XXXX-XXXX", palette::kTextDim.withAlpha(0.6f));
        keyInput_.setFont(fonts::value(16.0f));
        keyInput_.setJustification(juce::Justification::centred);
        keyInput_.setInputRestrictions(19); // 16 chars + 3 dashes
        keyInput_.onReturnKey = [this] { attemptActivate(); };
        addAndMakeVisible(keyInput_);

        statusLabel_.setFont(fonts::micro(10.0f));
        statusLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        statusLabel_.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(statusLabel_);
        setStatus(Status::Idle, "STATUS: READY TO SECURE CONNECTION");

        activateButton_.setColour(juce::TextButton::buttonColourId, branding_.accentColour);
        activateButton_.setColour(juce::TextButton::textColourOffId, palette::kFigmaBgDeep);
        activateButton_.onClick = [this] {
            if (status_ == Status::Success)
                dismiss();
            else
                attemptActivate();
        };
        addAndMakeVisible(activateButton_);

        getKeyLink_.setButtonText(branding_.getKeyLinkText);
        getKeyLink_.setURL(juce::URL(net::MurmurApiClient::defaultBaseUrl() + "/keys"));
        getKeyLink_.setFont(fonts::micro(11.0f), false);
        getKeyLink_.setColour(juce::HyperlinkButton::textColourId, branding_.accentColour);
        getKeyLink_.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(getKeyLink_);

        skipLink_.setButtonText("SKIP FOR NOW");
        skipLink_.setURL(juce::URL()); // no browser launch -- click handled below instead
        skipLink_.setFont(fonts::micro(10.0f), false);
        skipLink_.setColour(juce::HyperlinkButton::textColourId, palette::kTextDim);
        skipLink_.setJustificationType(juce::Justification::centred);
        skipLink_.onClick = [this] { dismiss(); };
        addAndMakeVisible(skipLink_);
    }

    KeyActivationOverlay::~KeyActivationOverlay() = default;

    void KeyActivationOverlay::attemptActivate()
    {
        const auto rawKey = keyInput_.getText().trim().toUpperCase();
        if (!looksLikeLicenseKey(rawKey))
        {
            setStatus(Status::Error, "KEY MUST LOOK LIKE XXXX-XXXX-XXXX-XXXX");
            return;
        }

        setStatus(Status::Activating, "ACTIVATING...");
        keyInput_.setEnabled(false);
        activateButton_.setEnabled(false);

        net::MurmurApiClient::activate(rawKey, [this, rawKey](net::ActivateResult result) {
            if (!result.success)
            {
                setStatus(Status::Error, result.errorMessage.toUpperCase());
                keyInput_.setEnabled(true);
                activateButton_.setEnabled(true);
                return;
            }

            content::LicenseInfo info;
            info.licenseKey = rawKey;
            info.displayName = result.displayName;
            info.email = result.email;
            info.entitled = result.entitled;
            info.isDevCurator = result.isCurator;
            info.activatedAt = juce::Time::getCurrentTime().toISO8601(true);
            licenseStore_.setInfo(info);

            setStatus(Status::FetchingLibrary, "SYNCING PATCH LIBRARY...");

            net::MurmurApiClient::fetchLibrary(rawKey, [this, displayName = result.displayName](net::LibraryResult libResult) {
                if (libResult.success && onLibraryFetched)
                    onLibraryFetched(libResult.patches);

                const auto who = displayName.isNotEmpty() ? displayName.toUpperCase() : "LICENSED";
                setStatus(Status::Success,
                         libResult.success ? "LICENSED - " + who + " - " + juce::String(libResult.patches.size()) + " PATCHES SYNCED"
                                            : "LICENSED - " + who + " (LIBRARY SYNC FAILED)");
                activateButton_.setButtonText("CONTINUE");
                activateButton_.setEnabled(true);
            });
        });
    }

    void KeyActivationOverlay::setStatus(Status status, const juce::String& message)
    {
        status_ = status;
        statusLabel_.setText(message, juce::dontSendNotification);
        statusLabel_.setColour(juce::Label::textColourId,
                              status == Status::Error     ? kDanger.withAlpha(kDangerAlpha)
                              : status == Status::Success ? branding_.accentColour
                                                            : palette::kTextDim);
        repaint();
    }

    void KeyActivationOverlay::dismiss()
    {
        setVisible(false);
        if (onDismissed)
            onDismissed();
    }

    void KeyActivationOverlay::paint(juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();

        g.fillAll(juce::Colour(0xff0d0f14));
        if (branding_.backgroundImage.isValid())
        {
            g.drawImage(branding_.backgroundImage, bounds, juce::RectanglePlacement::fillDestination);
            g.setColour(juce::Colours::black.withAlpha(0.62f)); // darker than the splash --
                                                                  // this screen has real
                                                                  // interactive controls to
                                                                  // stay legible against
            g.fillRect(bounds);
        }
        else
        {
            // No photo asset for this product (Undertow) -- same real
            // procedural radial-glow fallback SplashOverlay uses, tinted
            // with the product's own accent colour.
            const auto centre = juce::Point<float>(bounds.getCentreX(), bounds.getY() + 90.0f);
            juce::ColourGradient glow(branding_.accentColour.withAlpha(0.22f), centre,
                                       branding_.accentColour.withAlpha(0.0f),
                                       centre.translated(0.0f, bounds.getHeight() * 0.4f), true);
            g.setGradientFill(glow);
            g.fillRect(bounds);
        }

        // Mark alone -- no separate wordmark underneath: the window chrome/
        // title bar already establishes the brand, so a second text lockup
        // here was redundant.
        const float headerTop = bounds.getY() + 30.0f;
        constexpr float markSize = 88.0f;
        const juce::Rectangle<float> markBounds(bounds.getCentreX() - markSize * 0.5f, headerTop, markSize, markSize);
        if (branding_.markIcon.isValid())
            g.drawImageWithin(branding_.markIcon, (int)markBounds.getX(), (int)markBounds.getY(),
                              (int)markBounds.getWidth(), (int)markBounds.getHeight(),
                              juce::RectanglePlacement::centred);

        // Real panel card behind the interactive controls, matching the
        // Figma frame's dark card treatment.
        const auto panel = panelBounds();
        g.setColour(palette::kPanel.withAlpha(0.92f));
        g.fillRoundedRectangle(panel, 10.0f);
        g.setColour(palette::kBorder);
        g.drawRoundedRectangle(panel, 10.0f, 1.0f);

        // Footer -- same real version string the splash uses, not a
        // fabricated one.
        g.setFont(fonts::micro(9.0f));
        g.setColour(palette::kFigmaTextDim.withAlpha(0.7f));
        g.drawText(branding_.footerLeft, juce::Rectangle<float>(16.0f, bounds.getBottom() - 26.0f, 240.0f, 16.0f),
                  juce::Justification::centredLeft, false);
        g.drawText("v" + juce::String(JucePlugin_VersionString),
                  juce::Rectangle<float>(bounds.getWidth() - 100.0f, bounds.getBottom() - 26.0f, 84.0f, 16.0f),
                  juce::Justification::centredRight, false);
    }

    juce::Rectangle<float> KeyActivationOverlay::panelBounds() const
    {
        const auto bounds = getLocalBounds().toFloat();
        constexpr float panelWidth = 360.0f;
        // Taller when there's a real description line to fit (Undertow);
        // MURMUR's real default has none, so its panel height is unchanged.
        const float panelHeight = 264.0f + (branding_.description.isNotEmpty() ? 40.0f : 0.0f);
        // Shifted down a touch when a subtitle sits between the mark and the
        // panel, so the two don't crowd each other.
        const float verticalOffset = 20.0f + (branding_.subtitle.isNotEmpty() ? 16.0f : 0.0f);
        return juce::Rectangle<float>(bounds.getCentreX() - panelWidth * 0.5f,
                                       bounds.getCentreY() - panelHeight * 0.5f + verticalOffset, panelWidth,
                                       panelHeight);
    }

    void KeyActivationOverlay::resized()
    {
        if (branding_.subtitle.isNotEmpty())
        {
            const auto bounds = getLocalBounds().toFloat();
            constexpr float markSize = 88.0f;
            const float headerTop = bounds.getY() + 30.0f;
            const float subtitleTop = headerTop + markSize + 6.0f;
            subtitleLabel_.setBounds(
                juce::Rectangle<float>(bounds.getX(), subtitleTop, bounds.getWidth(), 16.0f).toNearestInt());
        }

        auto panel = panelBounds().toNearestInt().reduced(24, 20);

        panelTitleLabel_.setBounds(panel.removeFromTop(24));
        panel.removeFromTop(14);

        if (branding_.description.isNotEmpty())
        {
            descriptionLabel_.setBounds(panel.removeFromTop(36));
            panel.removeFromTop(10);
        }

        keyInput_.setBounds(panel.removeFromTop(38));
        panel.removeFromTop(10);

        statusLabel_.setBounds(panel.removeFromTop(18));
        panel.removeFromTop(14);

        activateButton_.setBounds(panel.removeFromTop(36).reduced(40, 0));
        panel.removeFromTop(12);

        getKeyLink_.setBounds(panel.removeFromTop(18));
        panel.removeFromTop(4);
        skipLink_.setBounds(panel.removeFromTop(16));
    }

} // namespace pw8::plugin::ui
