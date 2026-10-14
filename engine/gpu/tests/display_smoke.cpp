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
    return frames == 120 ? 0 : 1;
}
