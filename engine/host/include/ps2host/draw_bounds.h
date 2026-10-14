#pragma once
#include <algorithm>
#include <cmath>
#include "ps2gpu/gs_renderer.h"
namespace ps2host {
struct DrawBounds { std::uint32_t x0,x1,y0,y1; };
inline DrawBounds BoundsOf(const ps2gpu::GsVertex* v,std::size_t n,std::uint32_t width,std::uint32_t height) {
    float lo_x=float(width),hi_x=0,lo_y=float(height),hi_y=0;
    // GS screen coordinates originate in fixed-point integers and are finite.
    // floor(min(x)) == min(floor(x)); round each final bound once per primitive.
    for(std::size_t i=0;i<n;++i) {
        lo_x=std::min(lo_x,v[i].x);hi_x=std::max(hi_x,v[i].x);
        lo_y=std::min(lo_y,v[i].y);hi_y=std::max(hi_y,v[i].y);
    }
    return {std::uint32_t(std::clamp(std::floor(lo_x),0.0f,float(width))),
            std::uint32_t(std::clamp(std::ceil(hi_x),0.0f,float(width))),
            std::uint32_t(std::clamp(std::floor(lo_y),0.0f,float(height))),
            std::uint32_t(std::clamp(std::ceil(hi_y),0.0f,float(height)))};
}
}
