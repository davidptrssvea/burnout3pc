/* Diagnostic of the real display path using a captured game frame.
 * Report includes upload, processing, queue waits and presentation submission;
 * it is not a game FPS measurement. The same pixels are used for each mode.
 */
#include "ps2gpu/display.h"
#include <SDL3/SDL.h>
#include <chrono>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
int main(int argc,char** argv) {
    if(argc<3) { std::puts("usage: fsr_bench capture1x.bmp capture2x.bmp [visual-1x|visual-2x|visual-fsr1x|visual-fsr2x]"); return 1; }
    std::string error;
    auto display=ps2gpu::CreateVulkanDisplay("Burnout 3 FSR comparison",3840,2160,&error);
    if(!display) { std::printf("%s\n",error.c_str()); return 1; }
    display->SetVsync(false);
    std::printf("%s\n",display->Describe().c_str());
    for(int round=0;round<(argc>3?1:3);++round) for(int mode=0;mode<4;++mode) {
        if(argc>3) {
            const char* names[]={"visual-1x","visual-fsr1x","visual-2x","visual-fsr2x"};
            if(std::strcmp(argv[3],names[mode])) continue;
        }
        SDL_Surface* bmp=SDL_LoadBMP(argv[mode<2?1:2]);
        if(!bmp) { std::puts(SDL_GetError()); return 1; }
        SDL_Surface* rgba=SDL_ConvertSurface(bmp,SDL_PIXELFORMAT_BGRA32);
        SDL_DestroySurface(bmp);
        if(!rgba) return 1;
        std::vector<std::uint32_t> pixels(std::size_t(rgba->w)*rgba->h);
        for(int y=0;y<rgba->h;++y) std::memcpy(pixels.data()+y*rgba->w,(char*)rgba->pixels+y*rgba->pitch,rgba->w*4);
        const auto w=rgba->w,h=rgba->h;
        SDL_DestroySurface(rgba);
        display->SetFsr((mode&1)!=0,0.5f);
        for(int i=0;i<60;++i) if(!display->Pump() || !display->Present(pixels.data(),w,h)) return 1;
        // Readback is deliberately before the timed loop.
        if(argc>3) {
            std::vector<std::uint32_t> result; std::uint32_t rw=0,rh=0;
            if(!display->ReadProcessed(&result,&rw,&rh)) return 1;
            SDL_Surface* surface=SDL_CreateSurfaceFrom(rw,rh,SDL_PIXELFORMAT_RGBA32,result.data(),rw*4);
            if(!surface || !SDL_SaveBMP(surface,(std::string(argv[3])+".bmp").c_str())) return 1;
            SDL_DestroySurface(surface);
        }
        const auto start=std::chrono::steady_clock::now();
        const int frames=argc>4?0:(argc>3?3600:600);
        for(int i=0;i<frames;++i) {
            if(!display->Pump() || !display->Present(pixels.data(),w,h)) return argc>3?0:1;
            if(argc>3) SDL_Delay(16);
        }
        const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/std::max(frames,1);
        std::printf("round=%d mode=%s input=%dx%d display_path_ms=%.4f\n",round,mode&1?"FSR1":"standard",w,h,ms);
        std::fflush(stdout);
    }
    return 0;
}
