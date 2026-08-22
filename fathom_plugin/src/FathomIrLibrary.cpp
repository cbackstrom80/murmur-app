#include "FathomIrLibrary.h"

namespace pw8::fathom
{
    // Real filenames, exactly matching resources/impulse-responses/ -- keep
    // in sync if IRs are ever added/removed/renamed. Index order is what
    // FathomParamLayout.cpp's "irIndex" param (0..37) selects.
    const std::array<IrEntry, kNumBundledIrs> kBundledIrs = {{
        {"Block Inside", "Block Inside.wav", IrCategory::Room},
        {"Bottle Hall", "Bottle Hall.wav", IrCategory::Hall},
        {"Cement Blocks 1", "Cement Blocks 1.wav", IrCategory::Hall},
        {"Cement Blocks 2", "Cement Blocks 2.wav", IrCategory::Hall},
        {"Chateau de Logne, Outside", "Chateau de Logne, Outside.wav", IrCategory::Room},
        {"Conic Long Echo Hall", "Conic Long Echo Hall.wav", IrCategory::Hall},
        {"Deep Space", "Deep Space.wav", IrCategory::Room},
        {"Derlon Sanctuary", "Derlon Sanctuary.wav", IrCategory::Hall},
        {"Direct Cabinet N1", "Direct Cabinet N1.wav", IrCategory::Cabinet},
        {"Direct Cabinet N2", "Direct Cabinet N2.wav", IrCategory::Cabinet},
        {"Direct Cabinet N3", "Direct Cabinet N3.wav", IrCategory::Cabinet},
        {"Direct Cabinet N4", "Direct Cabinet N4.wav", IrCategory::Cabinet},
        {"Five Columns Long", "Five Columns Long.wav", IrCategory::Special},
        {"Five Columns", "Five Columns.wav", IrCategory::Special},
        {"French 18th Century Salon", "French 18th Century Salon.wav", IrCategory::Hall},
        {"Going Home", "Going Home.wav", IrCategory::Special},
        {"Greek 7 Echo Hall", "Greek 7 Echo Hall.wav", IrCategory::Hall},
        {"Highly Damped Large Room", "Highly Damped Large Room.wav", IrCategory::Room},
        {"In The Silo Revised", "In The Silo Revised.wav", IrCategory::Special},
        {"In The Silo", "In The Silo.wav", IrCategory::Special},
        {"Large Bottle Hall", "Large Bottle Hall.wav", IrCategory::Hall},
        {"Large Long Echo Hall", "Large Long Echo Hall.wav", IrCategory::Hall},
        {"Large Wide Echo Hall", "Large Wide Echo Hall.wav", IrCategory::Hall},
        {"Masonic Lodge", "Masonic Lodge.wav", IrCategory::Hall},
        {"Musikvereinsaal", "Musikvereinsaal.wav", IrCategory::Hall},
        {"Narrow Bumpy Space", "Narrow Bumpy Space.wav", IrCategory::Room},
        {"Nice Drum Room", "Nice Drum Room.wav", IrCategory::Room},
        {"On a Star", "On a Star.wav", IrCategory::Special},
        {"Parking Garage", "Parking Garage.wav", IrCategory::Special},
        {"Rays", "Rays.wav", IrCategory::Special},
        {"Right Glass Triangle", "Right Glass Triangle.wav", IrCategory::Special},
        {"Ruby Room", "Ruby Room.wav", IrCategory::Room},
        {"Scala Milan Opera Hall", "Scala Milan Opera Hall.wav", IrCategory::Hall},
        {"Small Drum Room", "Small Drum Room.wav", IrCategory::Room},
        {"Small Prehistoric Cave", "Small Prehistoric Cave.wav", IrCategory::Room},
        {"St Nicolaes Church", "St Nicolaes Church.wav", IrCategory::Hall},
        {"Trig Room", "Trig Room.wav", IrCategory::Room},
        {"Vocal Duo", "Vocal Duo.wav", IrCategory::Special},
    }};

    const char* irCategoryLabel(IrCategory category) noexcept
    {
        switch (category)
        {
            case IrCategory::Room: return "ROOM";
            case IrCategory::Hall: return "HALL / CHURCH";
            case IrCategory::Cabinet: return "CABINET";
            case IrCategory::Special: return "SPECIAL";
        }
        return "?";
    }

    juce::File impulseResponsesDirectory()
    {
        return juce::File::getSpecialLocation(juce::File::currentApplicationFile)
            .getParentDirectory()
            .getChildFile("Resources")
            .getChildFile("impulse-responses");
    }

} // namespace pw8::fathom
