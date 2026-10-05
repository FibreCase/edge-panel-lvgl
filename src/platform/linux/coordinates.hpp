#pragma once
#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <stdexcept>
namespace panel {
struct Point { int x, y; };
constexpr int logical_width=1280, logical_height=720;
constexpr int physical_width=720, physical_height=1280;
inline Point to_physical(Point p,int rotation) {
    if(rotation==90) return {physical_width-1-p.y,p.x};
    if(rotation==270) return {p.y,physical_height-1-p.x};
    throw std::invalid_argument("Rotation must be 90 or 270");
}
inline Point to_logical(Point p,int rotation) {
    if(rotation==90) return {p.y,physical_width-1-p.x};
    if(rotation==270) return {physical_height-1-p.y,p.x};
    throw std::invalid_argument("Rotation must be 90 or 270");
}
inline int normalize_axis(int value,int min,int max,int extent) {
    if(max<=min) throw std::invalid_argument("Invalid touch axis range");
    int64_t delta=static_cast<int64_t>(std::clamp(value,min,max))-min;
    return static_cast<int>(delta*(extent-1)/(static_cast<int64_t>(max)-min));
}
struct TouchTransform {
    int xmin=0,xmax=1,ymin=0,ymax=1,rotation=90;
    bool swap=false,invert_x=false,invert_y=false,logical=false;
    Point map(int raw_x,int raw_y) const {
        int x=raw_x,y=raw_y,x0=xmin,x1=xmax,y0=ymin,y1=ymax;
        if(swap) { std::swap(x,y);std::swap(x0,y0);std::swap(x1,y1); }
        const int w=logical?logical_width:physical_width,h=logical?logical_height:physical_height;
        x=normalize_axis(x,x0,x1,w);y=normalize_axis(y,y0,y1,h);
        if(invert_x) x=w-1-x;
        if(invert_y) y=h-1-y;
        return logical?Point{x,y}:to_logical({x,y},rotation);
    }
};
// Respect the kernel-provided scanout stride; XRGB8888 uses four bytes/pixel.
inline void rotate_frame(const uint32_t* source,uint8_t* destination,uint32_t pitch,int rotation) {
    if(pitch<physical_width*4) throw std::invalid_argument("Invalid scanout pitch");
    if(rotation!=90&&rotation!=270) throw std::invalid_argument("Invalid rotation");
    for(int py=0;py<physical_height;++py) {
        auto* row=reinterpret_cast<uint32_t*>(destination+static_cast<size_t>(py)*pitch);
        for(int px=0;px<physical_width;++px) {
            Point p=to_logical({px,py},rotation);
            row[px]=source[p.y*logical_width+p.x];
        }
    }
}
}
