#include <cstdio>
#include <cstring>
#include <set>
#include <cmath>
#include "ps2host/xbox_prompts.h"
#include "ps2host/settings.h"

namespace {
int failures=0;
void Check(bool ok,const char* text) { if(!ok) { ++failures; std::printf("FAIL: %s\n",text); } }
std::uint64_t Cross(const ps2host::texture_pack::Image& image) {
    std::uint64_t hash=1469598103934665603ull;
    for(int y=0;y<14*8;++y) for(int x=0;x<14*8;++x) for(int c=0;c<4;++c) {
        hash^=image.data[(y*image.width+x)*4+c]; hash*=1099511628211ull;
    }
    return hash;
}
}
int main(int argc,char** argv) {
    using namespace ps2host;
    std::vector<std::uint32_t> original(128*64,0);
    original[60*128+120]=0x80654321u;
    controls::Config config;
    std::set<std::uint64_t> fingerprints;
    for(int source=0;source<16;++source) {
        config.sources[14]=std::uint16_t(1u<<source);
        auto image=xbox_prompts::Build(config,original,128,64);
        Check(bool(image),"every Xbox button can be rendered");
        if(!image) continue;
        fingerprints.insert(Cross(*image));
        if(argc==2) { auto path=std::string(argv[1])+"."+std::to_string(source); if(auto out=std::fopen(path.c_str(),"wb")) { std::fwrite(image->data.data(),1,image->data.size(),out); std::fclose(out); } }
        Check(image->width==1024 && image->height==512 && image->format==0,"high-resolution RGBA atlas");
        std::uint32_t preserved=0;
        std::memcpy(&preserved,&image->data[((60*8)*image->width+120*8)*4],4);
        Check(preserved==original[60*128+120],"non-button artwork and GS alpha stay intact");
        for(std::size_t i=3;i<image->data.size();i+=4) if(image->data[i]>128) { Check(false,"alpha stays in GS range"); break; }
    }
    Check(fingerprints.size()==16,"all sixteen physical controls have distinct images");
    config.sources[14]=controls::ParseSources("a+rt",0);
    auto combined=xbox_prompts::Build(config,original,128,64);
    Check(combined && !fingerprints.count(Cross(*combined)),"A and RT are both shown as alternatives");
    config.sources[14]=0;
    auto none=xbox_prompts::Build(config,original,128,64);
    Check(none && !fingerprints.count(Cross(*none)),"an unbound action gets a disabled symbol");
    Check(!xbox_prompts::Build(config,original,64,64),"other textures are rejected");
    LoadSettings(""); SetSetting("map.cross","a+rt"); xbox_prompts::Initialize(); xbox_prompts::RegisterTexture(123);
    ps2gpu::GsVertex quad[3]{};
    quad[0].x=100; quad[0].y=400; quad[1].x=111; quad[1].y=400; quad[2].x=100; quad[2].y=411;
    for(auto& v:quad) { v.q=1; v.s=(v.x-100+2)/128; v.t=(v.y-400+3)/64; }
    Check(!xbox_prompts::Expand(999,quad,3,640,448),"unrelated game geometry stays unchanged");
    Check(xbox_prompts::Expand(123,quad,3,640,448),"alternatives get room at drawing time");
    Check(std::fabs(quad[0].x-71.4f)<.01f && quad[1].x==111 && quad[2].y<448,"both alternatives are readable and stay within the screen");
    SetSetting("map.cross","lt"); xbox_prompts::Initialize(); xbox_prompts::RegisterTexture(123);
    Check(xbox_prompts::Expand(123,quad,3,640,448),"a single trigger also gets a readable keycap");
    // Actual UV rectangles observed in the game differ for each PS2 face glyph.
    const float faceUv[][4]={{2,3,13,14},{15,2,28,15},{1,16,14,29},{15,16,28,29}};
    for(const auto& uv:faceUv) {
        ps2gpu::GsVertex hint[3]{};
        for(auto& v:hint) { v.q=1; v.x=550; v.y=420; v.s=uv[0]/128; v.t=uv[1]/64; }
        hint[1].x=563; hint[1].s=uv[2]/128; hint[2].y=433; hint[2].t=uv[3]/64;
        Check(xbox_prompts::Expand(123,hint,3,640,448),"each real face-glyph UV crop receives a readable button image");
    }
    SetSetting("xbox_prompts","off"); xbox_prompts::Initialize();
    Check(!xbox_prompts::HasTexture(123),"disabling prompts also clears registered textures");
    if(argc==2 && combined) {
        if(auto out=std::fopen(argv[1],"wb")) { std::fwrite(combined->data.data(),1,combined->data.size(),out); std::fclose(out); }
    }
    std::printf("Xbox prompt tests: %s\n",failures?"FAILED":"all passed");
    return failures?1:0;
}
