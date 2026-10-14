#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <string>

namespace ps2host::controls {
// Physical buttons are represented in libpad order, independently of SDL.
inline constexpr const char* Physical[16] = {
    "view", "l3", "r3", "menu", "up", "right", "down", "left",
    "lt", "rt", "lb", "rb", "y", "b", "a", "x"};
inline constexpr const char* Destination[16] = {
    "select", "l3", "r3", "start", "up", "right", "down", "left",
    "l2", "r2", "l1", "r1", "triangle", "circle", "cross", "square"};
struct Config {
    std::array<std::uint16_t,16> sources{};
    int trigger_threshold = 64;
    int rumble_percent = 100;
    Config() { for(int i=0;i<16;++i) sources[i] = std::uint16_t(1u<<i); }
};
inline std::uint16_t ParseSources(const std::string& value, std::uint16_t fallback) {
    if(value=="none") return 0;
    std::uint16_t result=0;
    std::size_t begin=0;
    while(begin<value.size()) {
        auto end=value.find('+',begin);
        const auto name=value.substr(begin,end==std::string::npos?end:end-begin);
        int found=-1;
        for(int i=0;i<16;++i) if(name==Physical[i]) found=i;
        if(found<0) return fallback; // Invalid configurations never silently lose an input.
        result |= std::uint16_t(1u<<found);
        if(end==std::string::npos) return result;
        begin=end+1;
        if(begin==value.size()) return fallback;
    }
    return fallback;
}
inline int Percent(const std::string& value,int fallback) {
    if(value.empty() || value.size()>3) return fallback;
    int n=0;
    for(char c:value) { if(c<'0'||c>'9') return fallback; n=n*10+c-'0'; }
    return std::clamp(n,0,100);
}
struct Result { std::uint16_t buttons=0; std::array<std::uint8_t,16> pressure{}; };
inline Result Map(const Config& config,std::uint16_t buttons,std::uint8_t lt,std::uint8_t rt) {
    std::array<std::uint8_t,16> physical{};
    for(int i=0;i<16;++i) physical[i]=(buttons&(1u<<i))?255:0;
    physical[8]=lt; physical[9]=rt;
    Result out;
    for(int dest=0;dest<16;++dest) for(int src=0;src<16;++src) {
        if(!(config.sources[dest]&(1u<<src))) continue;
        const auto pressure=physical[src];
        const bool held=(src==8||src==9)?pressure>config.trigger_threshold:pressure!=0;
        if(held) {
            out.buttons |= std::uint16_t(1u<<dest);
            out.pressure[dest]=std::max(out.pressure[dest],pressure);
        }
    }
    return out;
}
inline std::uint16_t Rumble(std::uint8_t input,int percent) {
    return std::uint16_t(unsigned(input)*257u*unsigned(std::clamp(percent,0,100))/100u);
}
} // namespace ps2host::controls
