#pragma once

#include <tesla.hpp>

namespace fz::ui {

// Keep the library's input handling and drawing geometry. The V2 slider handle
// ends at y + 69, so 74 px retains a five-pixel bottom inset and the full touch area.
template <typename Base>
class CompactSlider : public Base {
public:
    using Base::Base;

    void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
        Base::layout(parentX, parentY, parentWidth, parentHeight);
        this->setBoundaries(this->getX(), this->getY(), this->getWidth(), Height);
    }

    static constexpr u16 Height = 74;
};

using TrackBar = CompactSlider<tsl::elm::TrackBar>;
using StepTrackBar = CompactSlider<tsl::elm::StepTrackBar>;
using NamedStepTrackBar = CompactSlider<tsl::elm::NamedStepTrackBar>;

} // namespace fz::ui
