// 动态配色的无窗口测试：HCT 色彩空间、MD3 深色方案与种子轮换。
#include "ui/theme/color_scheme.hpp"
#include "ui/theme/material_color.hpp"
#include <cmath>
#include <cstdio>
#include <ctime>
#include <string>
#include <limits>
#include <stdexcept>
#include "cpp/cam/hct.h"
#include "cpp/scheme/scheme_tonal_spot.h"
namespace {
int failures = 0;
void check(bool condition, const std::string& what) {
    if(condition) return;
    std::fprintf(stderr, "FAIL %s\n", what.c_str());
    ++failures;
}
void check_near(double actual, double expected, double tolerance, const std::string& what) {
    if(std::fabs(actual - expected) <= tolerance) return;
    std::fprintf(stderr, "FAIL %s: got %.4f, want %.4f\n", what.c_str(), actual, expected);
    ++failures;
}
void check_rgb(panel::theme::Rgb actual, panel::theme::Rgb expected, const std::string& what) {
    for(int shift : {16, 8, 0}) {
        int got = (actual >> shift) & 0xFF, want = (expected >> shift) & 0xFF;
        if(std::abs(got - want) <= 1) continue;
        std::fprintf(stderr, "FAIL %s: got #%06X, want #%06X\n", what.c_str(), actual, expected);
        ++failures;
        return;
    }
}
void check_hct_round_trip(panel::theme::Rgb rgb) {
    auto hct = panel::theme::hct_from_rgb(rgb);
    check_rgb(panel::theme::rgb_from_hct(hct.hue, hct.chroma, hct.tone), rgb,
              "HCT round trip for #" + std::to_string(rgb));
}
}
int main() {
    using namespace panel::theme;

    // HCT 的 tone 就是 CIE L*。
    check_near(lstar_from_rgb(0x000000), 0.0, 0.1, "black L*");
    check_near(lstar_from_rgb(0xFFFFFF), 100.0, 0.1, "white L*");
    check_near(lstar_from_rgb(0x808080), 53.59, 0.1, "mid gray L*");
    // CAM16 的白点适应不会把中性色压到彩度 0，残余彩度应远小于任何色调板彩度。
    check(hct_from_rgb(0xFFFFFF).chroma < 4.0, "white chroma stays low");
    check(hct_from_rgb(0x808080).chroma < 4.0, "gray chroma stays low");

    // 已知锚点：MD3 基线种子 #6750A4 的 HCT。
    auto seed = hct_from_rgb(0x6750A4);
    check_near(seed.hue, 298.98, 0.05, "seed hue");
    check_near(seed.chroma, 47.8565, 0.005, "seed chroma");
    check_near(seed.tone, 40.08, 0.05, "seed tone");
    for(Rgb rgb : {0x6750A4u, 0x4285F4u, 0xE5EEE8u, 0x101713u, 0xFF0000u, 0x00FF00u, 0x0000FFu})
        check_hct_round_trip(rgb);

    // 对完整 60 分钟色环逐项验证领域角色到官方动态角色的映射。
    // 旧基线方案 C48 与官方 Tonal Spot C36 不能混用。
    for(int minute=0; minute<60; ++minute) {
        double hue=minute*6.0;
        material_color_utilities::SchemeTonalSpot reference(material_color_utilities::Hct(hue,48.0,50.0),true,0.0);
        auto actual=dark_scheme_from_seed_hue(hue);
        check_near(reference.primary_palette.get_chroma(),36.0,1e-9,"official primary chroma");
        check(actual.primary == (reference.GetPrimary() & 0xFFFFFFu),"official primary");
        check(actual.on_primary == (reference.GetOnPrimary() & 0xFFFFFFu),"official on_primary");
        check(actual.primary_container == (reference.GetPrimaryContainer() & 0xFFFFFFu),"official primary_container");
        check(actual.on_primary_container == (reference.GetOnPrimaryContainer() & 0xFFFFFFu),"official on_primary_container");
        check(actual.secondary == (reference.GetSecondary() & 0xFFFFFFu),"official secondary");
        check(actual.on_secondary == (reference.GetOnSecondary() & 0xFFFFFFu),"official on_secondary");
        check(actual.secondary_container == (reference.GetSecondaryContainer() & 0xFFFFFFu),"official secondary_container");
        check(actual.on_secondary_container == (reference.GetOnSecondaryContainer() & 0xFFFFFFu),"official on_secondary_container");
        check(actual.tertiary == (reference.GetTertiary() & 0xFFFFFFu),"official tertiary");
        check(actual.on_tertiary == (reference.GetOnTertiary() & 0xFFFFFFu),"official on_tertiary");
        check(actual.tertiary_container == (reference.GetTertiaryContainer() & 0xFFFFFFu),"official tertiary_container");
        check(actual.on_tertiary_container == (reference.GetOnTertiaryContainer() & 0xFFFFFFu),"official on_tertiary_container");
        check(actual.error == (reference.GetError() & 0xFFFFFFu),"official error");
        check(actual.on_error == (reference.GetOnError() & 0xFFFFFFu),"official on_error");
        check(actual.error_container == (reference.GetErrorContainer() & 0xFFFFFFu),"official error_container");
        check(actual.on_error_container == (reference.GetOnErrorContainer() & 0xFFFFFFu),"official on_error_container");
        check(actual.surface == (reference.GetSurface() & 0xFFFFFFu),"official surface");
        check(actual.on_surface == (reference.GetOnSurface() & 0xFFFFFFu),"official on_surface");
        check(actual.surface_variant == (reference.GetSurfaceVariant() & 0xFFFFFFu),"official surface_variant");
        check(actual.on_surface_variant == (reference.GetOnSurfaceVariant() & 0xFFFFFFu),"official on_surface_variant");
        check(actual.surface_container_lowest == (reference.GetSurfaceContainerLowest() & 0xFFFFFFu),"official surface_container_lowest");
        check(actual.surface_container_low == (reference.GetSurfaceContainerLow() & 0xFFFFFFu),"official surface_container_low");
        check(actual.surface_container == (reference.GetSurfaceContainer() & 0xFFFFFFu),"official surface_container");
        check(actual.surface_container_high == (reference.GetSurfaceContainerHigh() & 0xFFFFFFu),"official surface_container_high");
        check(actual.surface_container_highest == (reference.GetSurfaceContainerHighest() & 0xFFFFFFu),"official surface_container_highest");
        check(actual.outline == (reference.GetOutline() & 0xFFFFFFu),"official outline");
        check(actual.outline_variant == (reference.GetOutlineVariant() & 0xFFFFFFu),"official outline_variant");
        check(actual.inverse_surface == (reference.GetInverseSurface() & 0xFFFFFFu),"official inverse_surface");
        check(actual.inverse_on_surface == (reference.GetInverseOnSurface() & 0xFFFFFFu),"official inverse_on_surface");
        check(actual.inverse_primary == (reference.GetInversePrimary() & 0xFFFFFFu),"official inverse_primary");
        check(actual.background == (reference.GetBackground() & 0xFFFFFFu),"official background");
        check(actual.on_background == (reference.GetOnBackground() & 0xFFFFFFu),"official on_background");
        check(actual.surface_tint == (reference.GetSurfaceTint() & 0xFFFFFFu),"official surface_tint");
    }

    // 种子轮换：每分钟 6°，一小时正好一圈。用本地时间构造，避免受时区偏移影响。
    std::tm when{};
    when.tm_year = 2026 - 1900; when.tm_mon = 0; when.tm_mday = 1; when.tm_isdst = -1;
    auto hue_at_minute = [&](int minute) {
        when.tm_hour = 3; when.tm_min = minute; when.tm_sec = 0;
        return seed_hue_at(std::mktime(&when));
    };
    check_near(hue_at_minute(0), 0.0, 1e-9, "seed hue at :00");
    check_near(hue_at_minute(1), 6.0, 1e-9, "seed hue at :01");
    check_near(hue_at_minute(30), 180.0, 1e-9, "seed hue at :30");
    check_near(hue_at_minute(59), 354.0, 1e-9, "seed hue at :59");
    check_near(hue_at_minute(60), 0.0, 1e-9, "seed hue wraps next hour");

    // 任意色相下各角色都保持自己的 tone，且深浅关系成立。
    for(int degrees = 0; degrees < 360; degrees += 15) {
        ColorScheme scheme = dark_scheme_from_seed_hue(degrees);
        std::string tag = " at hue " + std::to_string(degrees);
        check_near(lstar_from_rgb(scheme.primary), 80.0, 0.6, "primary tone" + tag);
        check_near(lstar_from_rgb(scheme.surface), 6.0, 0.6, "surface tone" + tag);
        check_near(lstar_from_rgb(scheme.surface_container_high), 17.0, 0.6, "containerHigh tone" + tag);
        check_near(lstar_from_rgb(scheme.on_surface), 90.0, 0.6, "onSurface tone" + tag);
        check_near(lstar_from_rgb(scheme.on_primary_container), 90.0, 0.6, "onPrimaryContainer tone" + tag);
        check(lstar_from_rgb(scheme.on_primary_container) > lstar_from_rgb(scheme.primary_container),
              "container contrast" + tag);
        check(lstar_from_rgb(scheme.on_surface) > lstar_from_rgb(scheme.surface), "surface contrast" + tag);
        check(lstar_from_rgb(scheme.on_primary) < lstar_from_rgb(scheme.primary), "primary contrast" + tag);
    }
    // 相邻分钟的种子应产生不同配色，整圈后回到起点。
    check(dark_scheme_from_seed_hue(0).primary != dark_scheme_from_seed_hue(6).primary, "hue rotation changes colors");
    check(dark_scheme_from_seed_hue(0).primary == dark_scheme_from_seed_hue(360).primary, "hue wraps");

    check(dark_scheme_from_seed_hue(-6).primary == dark_scheme_from_seed_hue(354).primary, "negative hue wraps");
    for(double invalid : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}) {
        bool rejected=false;
        try { dark_scheme_from_seed_hue(invalid); } catch(const std::invalid_argument&) { rejected=true; }
        check(rejected,"scheme rejects nonfinite hue");
        rejected=false;
        try { rgb_from_hct(invalid,48,50); } catch(const std::invalid_argument&) { rejected=true; }
        check(rejected,"HCT rejects nonfinite hue");
    }
    // blend 按 sRGB 通道线性插值。
    check(blend(0x000000, 0xFFFFFF, 0.5) == 0x808080, "blend midpoint");
    check(blend(0x123456, 0x123456, 0.5) == 0x123456, "blend identical");
    check(blend(0x000000, 0xFFFFFF, 0.0) == 0x000000, "blend zero alpha");

    if(failures != 0) {
        std::fprintf(stderr, "%d theme test(s) failed\n", failures);
        return 1;
    }
    std::printf("Theme tests passed\n");
    return 0;
}
