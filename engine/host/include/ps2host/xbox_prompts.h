#pragma once
#include "ps2host/controls.h"
#include "ps2host/texture_pack.h"
#include "ps2gpu/gs_renderer.h"

namespace ps2host::xbox_prompts {
// Xbox prompts are independent of optional texture packs: their labels must
// continue to describe the active controls when Tab disables a texture pack.
void Initialize();
bool Enabled();
bool Matches(const texture_pack::Name& name);
void RegisterTexture(std::uint64_t key);
bool HasTexture(std::uint64_t key);
bool Expand(std::uint64_t key,ps2gpu::GsVertex* vertices,std::size_t count,
            std::uint32_t width,std::uint32_t height);
std::shared_ptr<const texture_pack::Image> Cached(const texture_pack::Name& name);
std::shared_ptr<texture_pack::Image> Build(const controls::Config& config,
    const std::vector<std::uint32_t>& original, std::uint32_t width, std::uint32_t height);
std::shared_ptr<const texture_pack::Image> Make(const texture_pack::Name& name,
    const std::vector<std::uint32_t>& original, std::uint32_t width, std::uint32_t height);
}
