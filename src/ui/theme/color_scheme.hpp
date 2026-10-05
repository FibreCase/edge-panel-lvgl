#pragma once
#include "ui/theme/material_color.hpp"
#include <ctime>
namespace panel {
namespace theme {
// Material Design 3 深色配色方案（tonal spot 变体）中界面用到的角色。
struct ColorScheme {
    Rgb primary, on_primary, primary_container, on_primary_container;
    Rgb secondary, on_secondary, secondary_container, on_secondary_container;
    Rgb tertiary, on_tertiary, tertiary_container, on_tertiary_container;
    Rgb error, on_error, error_container, on_error_container;
    Rgb surface, on_surface, surface_variant, on_surface_variant;
    Rgb surface_container_lowest, surface_container_low, surface_container, surface_container_high, surface_container_highest;
    Rgb outline, outline_variant;
    Rgb inverse_surface, inverse_on_surface, inverse_primary;
    Rgb background, on_background, surface_tint;
};
// 由种子色相（度）生成深色配色方案。
ColorScheme dark_scheme_from_seed_hue(double hue);
// 种子色相随时间轮换：每分钟 6°，一小时转满 360° 色环。
double seed_hue_at(std::time_t now);
}
}
