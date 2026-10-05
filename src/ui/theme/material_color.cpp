#include "ui/theme/material_color.hpp"
#include "cpp/cam/hct.h"
#include "cpp/utils/utils.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace panel::theme {
Hct hct_from_rgb(Rgb rgb) {
    material_color_utilities::Hct color(rgb | 0xFF000000u);
    return {color.get_hue(), color.get_chroma(), color.get_tone()};
}
Rgb rgb_from_hct(double hue, double chroma, double tone) {
    if(!std::isfinite(hue) || !std::isfinite(chroma) || !std::isfinite(tone))
        throw std::invalid_argument("HCT values must be finite");
    return material_color_utilities::Hct(hue, std::max(0.0, chroma), std::clamp(tone, 0.0, 100.0)).ToInt() & 0xFFFFFFu;
}
double lstar_from_rgb(Rgb rgb) { return material_color_utilities::LstarFromArgb(rgb | 0xFF000000u); }
Rgb blend(Rgb base, Rgb overlay, double alpha) {
    alpha = std::clamp(alpha, 0.0, 1.0);
    auto mix = [&](int shift) {
        double from = (base >> shift) & 0xFF, to = (overlay >> shift) & 0xFF;
        return static_cast<Rgb>(std::lround(from + (to - from) * alpha));
    };
    return mix(16) << 16 | mix(8) << 8 | mix(0);
}
}
