#include "CuratorReviewOverlay.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kRowHeight = 56;

        /// One visible row -- name/category/tags label + real UP/DOWN
        /// buttons. Recycled by juce::ListBox via Model::refreshComponentForRow,
        /// the standard JUCE pattern for interactive controls inside a list.
        class RowComponent : public juce::Component
        {
        public:
            explicit RowComponent(CuratorReviewOverlay& owner) : owner_(owner)
            {
                label_.setFont(fonts::value(12.0f));
                label_.setColour(juce::Label::textColourId, palette::kTextPrimary);
                addAndMakeVisible(label_);

                upButton_.setButtonText("UP");
                upButton_.setColour(juce::TextButton::buttonColourId, palette::kAccent.withAlpha(0.15f));
                upButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
                upButton_.onClick = [this] { owner_.voteOnRow(row_, true); };
                addAndMakeVisible(upButton_);

                downButton_.setButtonText("DOWN");
                downButton_.setColour(juce::TextButton::buttonColourId, palette::kEdgeFeedback.withAlpha(0.15f));
                downButton_.setColour(juce::TextButton::textColourOffId, palette::kEdgeFeedback);
                downButton_.onClick = [this] { owner_.voteOnRow(row_, false); };
                addAndMakeVisible(downButton_);
            }

            void setRow(int row, const juce::String& text)
            {
                row_ = row;
                label_.setText(text, juce::dontSendNotification);
            }

            void resized() override
            {
                auto bounds = getLocalBounds().reduced(8, 6);
                downButton_.setBounds(bounds.removeFromRight(64));
                bounds.removeFromRight(6);
                upButton_.setBounds(bounds.removeFromRight(64));
                bounds.removeFromRight(10);
                label_.setBounds(bounds);
            }

        private:
            CuratorReviewOverlay& owner_;
            int row_ = -1;
            juce::Label label_;
            juce::TextButton upButton_;
            juce::TextButton downButton_;
        };
    } // namespace

    class CuratorReviewOverlay::Model : public juce::ListBoxModel
    {
    public:
        explicit Model(CuratorReviewOverlay& owner) : owner_(owner) {}

        int getNumRows() override { return owner_.patches_.size(); }

        void paintListBoxItem(int, juce::Graphics& g, int width, int height, bool) override
        {
            g.setColour(palette::kBorder);
            g.drawHorizontalLine(height - 1, 0.0f, static_cast<float>(width));
        }

        juce::Component* refreshComponentForRow(int row, bool, juce::Component* existing) override
        {
            auto* rowComp = dynamic_cast<RowComponent*>(existing);
            if (rowComp == nullptr)
            {
                delete existing;
                rowComp = new RowComponent(owner_);
            }

            if (row >= 0 && row < owner_.patches_.size())
            {
                const auto& p = owner_.patches_.getReference(row);
                juce::String text = p.name + "   [" + p.category + "]";
                if (p.source == "manual")
                    text += "  (factory)";
                rowComp->setRow(row, text);
            }
            return rowComp;
        }

    private:
        CuratorReviewOverlay& owner_;
    };

    CuratorReviewOverlay::CuratorReviewOverlay()
    {
        setVisible(false);

        titleLabel_.setText("CURATOR REVIEW", juce::dontSendNotification);
        titleLabel_.setFont(fonts::title(15.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kFigmaTextPrimary);
        addAndMakeVisible(titleLabel_);

        statusLabel_.setFont(fonts::micro(10.0f));
        statusLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        statusLabel_.setText("Metadata only -- no audio preview yet.", juce::dontSendNotification);
        addAndMakeVisible(statusLabel_);

        closeButton_.onClick = [this] {
            setVisible(false);
            if (onDismissed)
                onDismissed();
        };
        addAndMakeVisible(closeButton_);

        model_ = std::make_unique<Model>(*this);
        listBox_.setModel(model_.get());
        listBox_.setRowHeight(kRowHeight);
        listBox_.setColour(juce::ListBox::backgroundColourId, palette::kPanel);
        addAndMakeVisible(listBox_);
    }

    CuratorReviewOverlay::~CuratorReviewOverlay() = default;

    void CuratorReviewOverlay::visibilityChanged()
    {
        if (isVisible())
            refresh();
    }

    void CuratorReviewOverlay::refresh()
    {
        statusLabel_.setText("Loading...", juce::dontSendNotification);
        const auto key = getLicenseKey ? getLicenseKey() : juce::String();

        net::MurmurApiClient::fetchCuratorQueue(key, [this](net::CuratorQueueResult result) {
            if (!result.success)
            {
                statusLabel_.setText(result.errorMessage, juce::dontSendNotification);
                return;
            }
            patches_ = result.patches;
            statusLabel_.setText(juce::String(patches_.size()) + " patches pending review -- metadata only, no audio preview yet.",
                                 juce::dontSendNotification);
            listBox_.updateContent();
        });
    }

    void CuratorReviewOverlay::voteOnRow(int row, bool voteUp)
    {
        if (row < 0 || row >= patches_.size())
            return;

        const auto slug = patches_.getReference(row).slug;
        const auto key = getLicenseKey ? getLicenseKey() : juce::String();

        net::MurmurApiClient::submitCuratorVote(key, slug, voteUp, [this, slug](net::CuratorVoteResult result) {
            if (!result.success)
            {
                statusLabel_.setText(result.errorMessage, juce::dontSendNotification);
                return;
            }
            // Voted rows leave the queue immediately (matches the server's
            // own "exclude already-voted" query) -- no need to wait for a
            // full re-fetch.
            for (int i = 0; i < patches_.size(); ++i)
            {
                if (patches_.getReference(i).slug == slug)
                {
                    patches_.remove(i);
                    break;
                }
            }
            listBox_.updateContent();
            statusLabel_.setText(juce::String(patches_.size()) + " patches pending review.", juce::dontSendNotification);
        });
    }

    void CuratorReviewOverlay::paint(juce::Graphics& g)
    {
        g.fillAll(palette::kFigmaBgDeep.withAlpha(0.97f));
    }

    void CuratorReviewOverlay::resized()
    {
        auto bounds = getLocalBounds().reduced(40);

        auto header = bounds.removeFromTop(40);
        closeButton_.setBounds(header.removeFromRight(80));
        titleLabel_.setBounds(header);

        statusLabel_.setBounds(bounds.removeFromTop(20));
        bounds.removeFromTop(8);

        listBox_.setBounds(bounds);
    }

} // namespace pw8::plugin::ui
