#include "FathomEditor.h"

#include "FathomIrLibrary.h"
#include "ui/theme/ObsidianPalette.h"
#include "ui/theme/ObsidianFonts.h"

namespace pw8::fathom
{
    namespace
    {
        constexpr int kWindowWidth = 760;
        constexpr int kWindowHeight = 660;
        constexpr int kKnobSize = 64;
        constexpr int kKnobRowHeight = 88;

        void styleSectionLabel(juce::Label& label, const juce::String& text)
        {
            label.setText(text, juce::dontSendNotification);
            label.setFont(plugin::ui::fonts::label(11.0f));
            label.setColour(juce::Label::textColourId, plugin::ui::palette::kTextDim);
            label.setJustificationType(juce::Justification::centredLeft);
        }
    } // namespace

    std::unique_ptr<juce::ComboBox> FathomEditor::makeIrComboBox()
    {
        auto box = std::make_unique<juce::ComboBox>();
        int itemId = 1;
        IrCategory lastCategory = kBundledIrs[0].category;
        box->addSectionHeading(irCategoryLabel(lastCategory));
        for (const auto& entry : kBundledIrs)
        {
            if (entry.category != lastCategory)
            {
                lastCategory = entry.category;
                box->addSectionHeading(irCategoryLabel(lastCategory));
            }
            box->addItem(entry.displayName, itemId++);
        }
        return box;
    }

    std::unique_ptr<FathomEditor::KnobControl> FathomEditor::makeKnob(const juce::String& paramId,
                                                                       const juce::String& labelText)
    {
        auto control = std::make_unique<KnobControl>();
        control->slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                                          juce::Slider::TextBoxBelow);
        control->slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, kKnobSize, 16);
        control->slider->setColour(juce::Slider::textBoxTextColourId, plugin::ui::palette::kTextPrimary);
        control->slider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(*control->slider);

        control->label = std::make_unique<juce::Label>();
        control->label->setText(labelText, juce::dontSendNotification);
        control->label->setFont(plugin::ui::fonts::micro(9.0f));
        control->label->setColour(juce::Label::textColourId, plugin::ui::palette::kTextDim);
        control->label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*control->label);

        control->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor_.getApvts(), paramId, *control->slider);

        return control;
    }

    FathomEditor::FathomEditor(FathomProcessor& processor)
        : juce::AudioProcessorEditor(&processor), processor_(processor)
    {
        setLookAndFeel(&laf_);

        titleLabel_.setText("FATHOM", juce::dontSendNotification);
        titleLabel_.setFont(plugin::ui::fonts::title(22.0f));
        titleLabel_.setColour(juce::Label::textColourId, plugin::ui::palette::kTextPrimary);
        addAndMakeVisible(titleLabel_);

        subtitleLabel_.setText("REVERB INSTRUMENT", juce::dontSendNotification);
        subtitleLabel_.setFont(plugin::ui::fonts::label(11.0f));
        subtitleLabel_.setColour(juce::Label::textColourId, plugin::ui::palette::kTextDim);
        addAndMakeVisible(subtitleLabel_);

        modeBox_ = std::make_unique<juce::ComboBox>();
        modeBox_->addItem("Algorithmic", 1);
        modeBox_->addItem("Convolution", 2);
        modeBox_->addItem("Hybrid", 3);
        addAndMakeVisible(*modeBox_);
        modeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor_.getApvts(), "reverbMode", *modeBox_);
        modeBox_->onChange = [this] { updateModeVisibility(); };
        styleSectionLabel(modeLabel_, "MODE");
        addAndMakeVisible(modeLabel_);

        addAndMakeVisible(algoPanel_);
        for (const auto& [paramId, label] : std::vector<std::pair<juce::String, juce::String>>{
                 {"mix", "MIX"},
                 {"reverbSizeParam", "SIZE"},
                 {"reverbDecaySeconds", "DECAY"},
                 {"reverbPreDelayMs", "PRE-DELAY"},
                 {"reverbDiffusion", "DIFFUSION"},
                 {"reverbDensity", "DENSITY"},
                 {"reverbEarlyLevel", "EARLY LVL"},
                 {"reverbLateLevel", "LATE LVL"},
                 {"reverbHighRatio", "HF RT MULT"},
                 {"reverbHighCrossoverHz", "HF XOVER"},
                 {"reverbLowRatio", "LF RT MULT"},
                 {"reverbLowCrossoverHz", "LF XOVER"},
                 {"reverbModDepth", "MOD DEPTH"},
                 {"reverbModRateHz", "MOD RATE"},
                 {"reverbRollOffHz", "ROLL OFF"},
                 {"reverbVlfCutDb", "VLF CUT"},
                 {"reverbShimmerAmount", "SHIMMER"},
             })
        {
            auto knob = makeKnob(paramId, label);
            algoPanel_.addAndMakeVisible(*knob->slider);
            algoPanel_.addAndMakeVisible(*knob->label);
            algoKnobs_.push_back(std::move(knob));
        }

        styleSectionLabel(characterLabel_, "CHARACTER");
        algoPanel_.addAndMakeVisible(characterLabel_);
        characterBox_ = std::make_unique<juce::ComboBox>();
        characterBox_->addItem("Default", 1);
        characterBox_->addItem("Plate", 2);
        characterBox_->addItem("Hall", 3);
        characterBox_->addItem("Room", 4);
        characterBox_->addItem("Spring", 5);
        characterBox_->addItem("Shimmer", 6);
        algoPanel_.addAndMakeVisible(*characterBox_);
        characterAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor_.getApvts(), "reverbCharacter", *characterBox_);

        addAndMakeVisible(convPanel_);
        for (const auto& [paramId, label] : std::vector<std::pair<juce::String, juce::String>>{
                 {"convPreDelayMs", "PRE-DELAY"},
                 {"convMix", "MIX"},
                 {"convWidth", "WIDTH"},
                 {"convLowCutHz", "LOW CUT"},
                 {"convHighCutHz", "HIGH CUT"},
             })
        {
            auto knob = makeKnob(paramId, label);
            convPanel_.addAndMakeVisible(*knob->slider);
            convPanel_.addAndMakeVisible(*knob->label);
            convKnobs_.push_back(std::move(knob));
        }

        styleSectionLabel(irLabel_, "IMPULSE RESPONSE");
        convPanel_.addAndMakeVisible(irLabel_);
        irBox_ = makeIrComboBox();
        convPanel_.addAndMakeVisible(*irBox_);
        // Real, honest note: ComboBox item IDs are 1-based and sequential in
        // real bundled-IR order here, matching the real `irIndex` APVTS
        // param's own 0..37 range (id - 1 == irIndex) -- ComboBoxAttachment
        // handles that offset the same way it already does for
        // reverbCharacter above.
        irAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor_.getApvts(), "irIndex", *irBox_);

        // -- Phase 2: real Hybrid panel -- real IR-derived early
        //    reflections (own picker, same real irIndex param) blended
        //    with the real algorithmic late tank. --
        addAndMakeVisible(hybridPanel_);

        styleSectionLabel(hybridIrLabel_, "IMPULSE RESPONSE (EARLY)");
        hybridPanel_.addAndMakeVisible(hybridIrLabel_);
        hybridIrBox_ = makeIrComboBox();
        hybridPanel_.addAndMakeVisible(*hybridIrBox_);
        hybridIrAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor_.getApvts(), "irIndex", *hybridIrBox_);

        for (const auto& [paramId, label] : std::vector<std::pair<juce::String, juce::String>>{
                 {"hybridEarlyLengthMs", "EARLY LENGTH"},
                 {"reverbEarlyLevel", "EARLY (IR) LVL"},
                 {"reverbLateLevel", "LATE (TANK) LVL"},
                 {"reverbSizeParam", "SIZE"},
                 {"reverbDecaySeconds", "DECAY"},
                 {"reverbPreDelayMs", "PRE-DELAY"},
                 {"reverbDiffusion", "DIFFUSION"},
                 {"reverbDensity", "DENSITY"},
                 {"reverbHighRatio", "HF RT MULT"},
                 {"reverbHighCrossoverHz", "HF XOVER"},
                 {"reverbLowRatio", "LF RT MULT"},
                 {"reverbLowCrossoverHz", "LF XOVER"},
                 {"reverbModDepth", "MOD DEPTH"},
                 {"reverbModRateHz", "MOD RATE"},
                 {"reverbShimmerAmount", "SHIMMER"},
             })
        {
            auto knob = makeKnob(paramId, label);
            hybridPanel_.addAndMakeVisible(*knob->slider);
            hybridPanel_.addAndMakeVisible(*knob->label);
            hybridKnobs_.push_back(std::move(knob));
        }

        styleSectionLabel(hybridCharacterLabel_, "CHARACTER");
        hybridPanel_.addAndMakeVisible(hybridCharacterLabel_);
        hybridCharacterBox_ = std::make_unique<juce::ComboBox>();
        hybridCharacterBox_->addItem("Default", 1);
        hybridCharacterBox_->addItem("Plate", 2);
        hybridCharacterBox_->addItem("Hall", 3);
        hybridCharacterBox_->addItem("Room", 4);
        hybridCharacterBox_->addItem("Spring", 5);
        hybridCharacterBox_->addItem("Shimmer", 6);
        hybridPanel_.addAndMakeVisible(*hybridCharacterBox_);
        hybridCharacterAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor_.getApvts(), "reverbCharacter", *hybridCharacterBox_);

        updateModeVisibility();
        setSize(kWindowWidth, kWindowHeight);
    }

    FathomEditor::~FathomEditor()
    {
        setLookAndFeel(nullptr);
    }

    void FathomEditor::updateModeVisibility()
    {
        const int mode = modeBox_->getSelectedId() - 1; // 0=Algorithmic, 1=Convolution, 2=Hybrid
        algoPanel_.setVisible(mode == 0);
        convPanel_.setVisible(mode == 1);
        hybridPanel_.setVisible(mode == 2);
    }

    void FathomEditor::paint(juce::Graphics& g)
    {
        g.fillAll(plugin::ui::palette::kFigmaBgDeep);
    }

    void FathomEditor::layoutKnobGrid(juce::Rectangle<int> area, const std::vector<KnobControl*>& knobs, int cols)
    {
        const int rows = (static_cast<int>(knobs.size()) + cols - 1) / cols;
        const int cellWidth = area.getWidth() / juce::jmax(1, cols);
        for (int i = 0; i < static_cast<int>(knobs.size()); ++i)
        {
            const int col = i % cols;
            const int row = i / cols;
            auto cell = juce::Rectangle<int>(area.getX() + col * cellWidth, area.getY() + row * kKnobRowHeight,
                                              cellWidth, kKnobRowHeight);
            auto* knob = knobs[static_cast<std::size_t>(i)];
            auto knobBounds = cell.removeFromTop(kKnobRowHeight - 14).withSizeKeepingCentre(kKnobSize, kKnobRowHeight - 14);
            knob->slider->setBounds(knobBounds);
            knob->label->setBounds(cell);
        }
        juce::ignoreUnused(rows);
    }

    void FathomEditor::resized()
    {
        auto bounds = getLocalBounds().reduced(20);

        auto header = bounds.removeFromTop(56);
        titleLabel_.setBounds(header.removeFromTop(30));
        subtitleLabel_.setBounds(header);

        bounds.removeFromTop(8);
        auto modeRow = bounds.removeFromTop(32);
        modeLabel_.setBounds(modeRow.removeFromLeft(50));
        modeRow.removeFromLeft(8);
        modeBox_->setBounds(modeRow.removeFromLeft(160).reduced(0, 4));

        bounds.removeFromTop(12);

        algoPanel_.setBounds(bounds);
        convPanel_.setBounds(bounds);
        hybridPanel_.setBounds(bounds);

        auto algoContent = algoPanel_.getContentBounds();
        std::vector<KnobControl*> algoPtrs;
        for (auto& k : algoKnobs_)
            algoPtrs.push_back(k.get());
        auto characterRow = algoContent.removeFromBottom(40);
        characterLabel_.setBounds(characterRow.removeFromLeft(90));
        characterBox_->setBounds(characterRow.removeFromLeft(160).reduced(0, 6));
        layoutKnobGrid(algoContent, algoPtrs, 5); // 17 real knobs (Phase 3 added Shimmer) -- 5 cols keeps this to 4 rows, matching the panel's real available height

        auto convContent = convPanel_.getContentBounds();
        std::vector<KnobControl*> convPtrs;
        for (auto& k : convKnobs_)
            convPtrs.push_back(k.get());
        auto irRow = convContent.removeFromTop(40);
        irLabel_.setBounds(irRow.removeFromLeft(120));
        irBox_->setBounds(irRow.reduced(0, 6));
        convContent.removeFromTop(8);
        layoutKnobGrid(convContent, convPtrs, 5);

        auto hybridContent = hybridPanel_.getContentBounds();
        std::vector<KnobControl*> hybridPtrs;
        for (auto& k : hybridKnobs_)
            hybridPtrs.push_back(k.get());
        auto hybridIrRow = hybridContent.removeFromTop(40);
        hybridIrLabel_.setBounds(hybridIrRow.removeFromLeft(150));
        hybridIrBox_->setBounds(hybridIrRow.reduced(0, 6));
        hybridContent.removeFromTop(4);
        auto hybridCharacterRow = hybridContent.removeFromBottom(40);
        hybridCharacterLabel_.setBounds(hybridCharacterRow.removeFromLeft(90));
        hybridCharacterBox_->setBounds(hybridCharacterRow.removeFromLeft(160).reduced(0, 6));
        layoutKnobGrid(hybridContent, hybridPtrs, 4);
    }

} // namespace pw8::fathom
