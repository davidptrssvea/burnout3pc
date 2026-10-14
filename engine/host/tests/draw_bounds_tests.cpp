#include <cstdio>
#include <chrono>
#include <array>
#include "ps2host/draw_bounds.h"
using ps2host::DrawBounds;
__declspec(noinline) DrawBounds Reference(const ps2gpu::GsVertex* v,std::size_t n,unsigned w,unsigned h) {
    DrawBounds b{w,0,h,0};for(std::size_t i=0;i<n;++i) {
        b.x0=std::min(b.x0,unsigned(std::clamp(std::floor(v[i].x),0.0f,float(w))));
        b.x1=std::max(b.x1,unsigned(std::clamp(std::ceil(v[i].x),0.0f,float(w))));
        b.y0=std::min(b.y0,unsigned(std::clamp(std::floor(v[i].y),0.0f,float(h))));
        b.y1=std::max(b.y1,unsigned(std::clamp(std::ceil(v[i].y),0.0f,float(h))));
    }return b;
}
__declspec(noinline) DrawBounds Optimized(const ps2gpu::GsVertex* v,std::size_t n,unsigned w,unsigned h) {return ps2host::BoundsOf(v,n,w,h);}
int main(){
    std::array<std::array<ps2gpu::GsVertex,6>,4096> data{};unsigned seed=17;
    auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return float(int(seed%131072)-65536)/16;};
    for(auto& p:data)for(auto& v:p){v.x=random();v.y=random();}
    for(auto n:{3u,6u})for(auto w:{64u,640u,2048u})for(auto h:{64u,448u,1024u})for(auto& p:data){
        auto a=Reference(p.data(),n,w,h),b=Optimized(p.data(),n,w,h);
        if(a.x0!=b.x0||a.x1!=b.x1||a.y0!=b.y0||a.y1!=b.y1){std::puts("FAIL: coverage differs");return 1;}
    }
    volatile unsigned sum=0;
    for(auto fn:{Reference,Optimized}){
        auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<3000000;++i){auto b=fn(data[i&4095].data(),6,640,448);sum=sum+b.x0+b.x1+b.y0+b.y1;}
        std::printf("%s bounds: %.2f ms / 3 million sprites\n",fn==Reference?"reference":"optimized",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
    }
    std::puts("Bounds: 73728 randomized clipped primitive comparisons passed");return 0;
}
