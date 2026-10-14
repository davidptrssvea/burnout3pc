/*
 * The display's smoke test: open the window, present a moving gradient at the
 * console's size for two seconds, close. Exit status 0 when every frame was
 * presented; prints the GPU it ran on.
 */

#include <cstdio>
#include <vector>

#include <SDL3/SDL.h>

#include "ps2gpu/display.h"

int main(int, char**) {
    std::string error;
    auto display = ps2gpu::CreateVulkanDisplay("ps2gpu smoke test", 1280, 896, &error);
    if (!display) {
        std::printf("display: %s\n", error.c_str());
        return 1;
    }
    std::printf("display: %s\n", display->Describe().c_str());
    std::uint32_t w = 640, h = 448;
    std::vector<std::uint32_t> pixels(std::size_t(w) * h);
    int frames = 0;
    for (; frames < 120; ++frames) {
        if (!display->Pump()) break;
        // Exercise allocation, reuse and switching back to the plain blit.
        display->SetFxaa(frames >= 20 && frames < 100);
        display->SetFsr(frames >= 30 && frames < 110, frames < 80 ? 0.0f : 1.0f);
        if (frames == 60) { w=639; h=447; pixels.resize(std::size_t(w)*h); }
        for (std::uint32_t y = 0; y < h; ++y)
            for (std::uint32_t x = 0; x < w; ++x)
                pixels[y * w + x] = 0xFF000000u | ((x + frames * 4) & 0xFF) << 16 | (y & 0xFF) << 8 | 0x40;
        if (!display->Present(pixels.data(), w, h)) {
            std::printf("present failed at frame %d\n", frames);
            return 1;
        }
        SDL_Delay(16);
    }
    std::printf("presented %d frames\n", frames);
    const double fps=display->PresentationFps();
    display->TakePacing();
    if (fps<=0.0 || fps>90.0 || display->PresentationFps()!=fps) {
        std::printf("FPS counter failed: %.1f\n",fps); return 1;
    }
    std::printf("FPS counter: %.1f (diagnostic sampling keeps it intact)\n",fps);
    // Solid-color readback exercises both FSR stages, corner clamping and odd
    // input dimensions; then aspect change reallocates the output resources.
    display->SetFxaa(false);
    pixels.assign(33*17,0xff205080u);  // BGRA input -> RGBA output 0xff805020
    for(int test=0;test<3;++test) {
        display->SetFsr(test!=2,0.5f);
        if(test==1) display->SetAspect(16.0f/9.0f);
        if(!display->Present(pixels.data(),33,17)) return 1;
        std::vector<std::uint32_t> result; std::uint32_t rw=0,rh=0;
        if(!display->ReadProcessed(&result,&rw,&rh)) return 1;
        if((test==0 && (rw!=1195 || rh!=896)) || (test==1 && (rw!=1280 || rh!=720)) || (test==2 && (rw!=33 || rh!=17))) {
            std::printf("FSR dimensions wrong: %ux%u\n",rw,rh); return 1;
        }
        for(auto pixel:result) for(int channel=0;channel<4;++channel) {
            const int expected=int((0xff805020u>>(channel*8))&255);
            const int actual=int((pixel>>(channel*8))&255);
            if(actual<expected-1 || actual>expected+1) { std::printf("FSR solid pixel wrong: %08x\n",pixel); return 1; }
        }
    }
    std::puts("FSR pixel readback, aspect and disabled passthrough passed");
    return frames == 120 ? 0 : 1;
}
