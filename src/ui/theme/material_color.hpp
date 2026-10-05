#pragma once
#include <cstdint>
namespace panel {
namespace theme {
// sRGB 颜色，打包为 0xRRGGBB。
using Rgb = uint32_t;
// HCT：Material Design 3 使用的颜色空间，由 CAM16 的色相/彩度与 L* 明度组成。
struct Hct { double hue, chroma, tone; };
// 由 RGB 求 HCT。
Hct hct_from_rgb(Rgb rgb);
// 通过官方 HCT solver 求 sRGB 色域内的 RGB；非有限输入抛出异常。
Rgb rgb_from_hct(double hue, double chroma, double tone);
// CIE L* 明度，与 HCT 的 tone 定义一致。
double lstar_from_rgb(Rgb rgb);
// 在 base 上叠加 overlay，alpha 为 overlay 的不透明度，按 sRGB 通道线性混合。
Rgb blend(Rgb base, Rgb overlay, double alpha);
}
}
