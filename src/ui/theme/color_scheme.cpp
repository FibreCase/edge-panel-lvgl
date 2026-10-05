#include "ui/theme/color_scheme.hpp"
#include "cpp/cam/hct.h"
#include "cpp/scheme/scheme_tonal_spot.h"
#include <cmath>
#include <stdexcept>
namespace panel::theme {
ColorScheme dark_scheme_from_seed_hue(double hue) {
    if(!std::isfinite(hue)) throw std::invalid_argument("Seed hue must be finite");
    hue = std::fmod(hue, 360.0);
    if(hue < 0.0) hue += 360.0;
    // Seed uses HCT C48/T50; the official Tonal Spot scheme sets primary C36.
    material_color_utilities::SchemeTonalSpot source(material_color_utilities::Hct(hue, 48.0, 50.0), true, 0.0);
    ColorScheme scheme{};
    scheme.primary = source.GetPrimary() & 0xFFFFFFu;
    scheme.on_primary = source.GetOnPrimary() & 0xFFFFFFu;
    scheme.primary_container = source.GetPrimaryContainer() & 0xFFFFFFu;
    scheme.on_primary_container = source.GetOnPrimaryContainer() & 0xFFFFFFu;
    scheme.secondary = source.GetSecondary() & 0xFFFFFFu;
    scheme.on_secondary = source.GetOnSecondary() & 0xFFFFFFu;
    scheme.secondary_container = source.GetSecondaryContainer() & 0xFFFFFFu;
    scheme.on_secondary_container = source.GetOnSecondaryContainer() & 0xFFFFFFu;
    scheme.tertiary = source.GetTertiary() & 0xFFFFFFu;
    scheme.on_tertiary = source.GetOnTertiary() & 0xFFFFFFu;
    scheme.tertiary_container = source.GetTertiaryContainer() & 0xFFFFFFu;
    scheme.on_tertiary_container = source.GetOnTertiaryContainer() & 0xFFFFFFu;
    scheme.error = source.GetError() & 0xFFFFFFu;
    scheme.on_error = source.GetOnError() & 0xFFFFFFu;
    scheme.error_container = source.GetErrorContainer() & 0xFFFFFFu;
    scheme.on_error_container = source.GetOnErrorContainer() & 0xFFFFFFu;
    scheme.surface = source.GetSurface() & 0xFFFFFFu;
    scheme.on_surface = source.GetOnSurface() & 0xFFFFFFu;
    scheme.surface_variant = source.GetSurfaceVariant() & 0xFFFFFFu;
    scheme.on_surface_variant = source.GetOnSurfaceVariant() & 0xFFFFFFu;
    scheme.surface_container_lowest = source.GetSurfaceContainerLowest() & 0xFFFFFFu;
    scheme.surface_container_low = source.GetSurfaceContainerLow() & 0xFFFFFFu;
    scheme.surface_container = source.GetSurfaceContainer() & 0xFFFFFFu;
    scheme.surface_container_high = source.GetSurfaceContainerHigh() & 0xFFFFFFu;
    scheme.surface_container_highest = source.GetSurfaceContainerHighest() & 0xFFFFFFu;
    scheme.outline = source.GetOutline() & 0xFFFFFFu;
    scheme.outline_variant = source.GetOutlineVariant() & 0xFFFFFFu;
    scheme.inverse_surface = source.GetInverseSurface() & 0xFFFFFFu;
    scheme.inverse_on_surface = source.GetInverseOnSurface() & 0xFFFFFFu;
    scheme.inverse_primary = source.GetInversePrimary() & 0xFFFFFFu;
    scheme.background = source.GetBackground() & 0xFFFFFFu;
    scheme.on_background = source.GetOnBackground() & 0xFFFFFFu;
    scheme.surface_tint = source.GetSurfaceTint() & 0xFFFFFFu;
    return scheme;
}
double seed_hue_at(std::time_t now) {
    std::tm local{};
    localtime_r(&now, &local);
    return local.tm_min * 6.0;
}
}
