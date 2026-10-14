#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <unordered_set>
#include "ps2host/xbox_prompts.h"
#include "ps2host/settings.h"

namespace ps2host::xbox_prompts {
namespace {
constexpr int kScale=8, kWidth=128*kScale, kHeight=64*kScale;
controls::Config config;
bool enabled=false;
std::map<std::string,std::shared_ptr<const texture_pack::Image>> images;
std::unordered_set<std::uint64_t> textures;
std::uint64_t next_id=0x58424f5800000000ull;
struct Gdi {
    ULONG_PTR token=0;
    Gdi() { Gdiplus::GdiplusStartupInput input; Gdiplus::GdiplusStartup(&token,&input,nullptr); }
    ~Gdi() { if(token) Gdiplus::GdiplusShutdown(token); }
};

// Rectangles are in the original game's 128x64 button atlas. The other
// artwork stays intact; the old decorative circle behind buttons is cleared.
struct Slot { int x,y,w,h,destination; };
constexpr Slot slots[] = {
    {2,3,11,11,14}, {15,2,13,13,12}, {1,16,13,13,13}, {15,16,13,13,15},
    {29,0,19,14,3}, {29,14,19,16,0}, {49,0,14,16,3},
    {84,0,24,20,3}, {65,20,62,18,3}, {84,40,24,16,3}
};
std::pair<int,int> Grid(std::uint16_t sources) {
    int count=0; for(int i=0;i<16;++i) count+=(sources>>i)&1;
    int columns=count<=3?std::max(1,count):count<=4?2:count<=9?3:4;
    return {columns,std::max(1,(count+columns-1)/columns)};
}

Gdiplus::Color Accent(int source) {
    using Gdiplus::Color;
    switch(source) {
        case 14: return Color(255,98,220,97); // A
        case 13: return Color(255,250,85,87); // B
        case 15: return Color(255,74,174,255); // X
        case 12: return Color(255,255,209,69); // Y
        default: return Color(255,238,243,250);
    }
}

void Rounded(Gdiplus::GraphicsPath& path,const Gdiplus::RectF& r,float radius) {
    float d=std::min(radius*2,std::min(r.Width,r.Height));
    path.AddArc(r.X,r.Y,d,d,180,90); path.AddArc(r.X+r.Width-d,r.Y,d,d,270,90);
    path.AddArc(r.X+r.Width-d,r.Y+r.Height-d,d,d,0,90); path.AddArc(r.X,r.Y+r.Height-d,d,d,90,90);
    path.CloseFigure();
}

void Button(Gdiplus::Graphics& g,Gdiplus::RectF bounds,int source) {
    using namespace Gdiplus;
    const bool face=source>=12 && source<=15;
    float inset=std::max(1.0f,std::min(bounds.Width,bounds.Height)*0.045f);
    bounds.Inflate(-inset,-inset);
    if(face || source<=7) { float side=std::min(bounds.Width,bounds.Height); bounds.X+=(bounds.Width-side)/2;
        bounds.Y+=(bounds.Height-side)/2; bounds.Width=bounds.Height=side; }
    else { float width=std::min(bounds.Width,bounds.Height*1.6f); bounds.X+=(bounds.Width-width)/2; bounds.Width=width; }
    GraphicsPath path;
    if(face || source==1 || source==2) path.AddEllipse(bounds);
    else Rounded(path,bounds,std::min(bounds.Width,bounds.Height)*0.22f);
    LinearGradientBrush fill(PointF(bounds.X,bounds.Y),PointF(bounds.X,bounds.Y+bounds.Height),
        Color(255,61,68,80),Color(255,15,19,27));
    g.FillPath(&fill,&path);
    Pen rim(Accent(source),std::max(1.0f,std::min(bounds.Width,bounds.Height)*0.035f));
    g.DrawPath(&rim,&path);
    const RectF inner(bounds.X+bounds.Width*.22f,bounds.Y+bounds.Height*.22f,bounds.Width*.56f,bounds.Height*.56f);
    SolidBrush white(Color(255,244,247,255));
    if(source==3) { // Menu: three horizontal bars.
        for(int i=0;i<3;++i) g.FillRectangle(&white,inner.X,inner.Y+inner.Height*i/3,inner.Width,inner.Height*.13f);
    } else if(source==0) { // View: two overlapping windows.
        Pen p(Color(255,244,247,255),std::max(1.0f,inner.Height*.08f));
        g.DrawRectangle(&p,inner.X,inner.Y+inner.Height*.3f,inner.Width*.7f,inner.Height*.65f);
        g.DrawRectangle(&p,inner.X+inner.Width*.3f,inner.Y,inner.Width*.7f,inner.Height*.65f);
    } else if(source>=4 && source<=7) {
        // A D-pad key's direction, rotated from an upward arrow.
        PointF points[7]={{0,-.5f},{.5f,0},{.18f,0},{.18f,.5f},{-.18f,.5f},{-.18f,0},{-.5f,0}};
        int turns=source==4?0:source==5?1:source==6?2:3;
        for(auto& point:points) { for(int i=0;i<turns;++i) { float x=point.X; point.X=-point.Y; point.Y=x; }
            point.X=inner.X+inner.Width*(point.X+.5f); point.Y=inner.Y+inner.Height*(point.Y+.5f); }
        g.FillPolygon(&white,points,7);
    } else {
        static const wchar_t* labels[16]={L"",L"LS",L"RS",L"",L"",L"",L"",L"",L"LT",L"RT",L"LB",L"RB",L"Y",L"B",L"A",L"X"};
        FontFamily family(L"Segoe UI");
        float height=std::min(bounds.Height*(face?.66f:.49f),bounds.Width*(face?.7f:.47f));
        Font font(&family,height,FontStyleBold,UnitPixel);
        StringFormat format; format.SetAlignment(StringAlignmentCenter); format.SetLineAlignment(StringAlignmentCenter);
        SolidBrush text(Accent(source));
        g.DrawString(labels[source],-1,&font,bounds,&format,&text);
        if(source==1 || source==2) { // Stick clicks: a small downward chevron.
            Pen p(Color(255,244,247,255),std::max(1.0f,bounds.Height*.025f));
            g.DrawLine(&p,bounds.X+bounds.Width*.42f,bounds.Y+bounds.Height*.79f,bounds.X+bounds.Width*.5f,bounds.Y+bounds.Height*.87f);
            g.DrawLine(&p,bounds.X+bounds.Width*.5f,bounds.Y+bounds.Height*.87f,bounds.X+bounds.Width*.58f,bounds.Y+bounds.Height*.79f);
        }
    }
}

void Alternatives(Gdiplus::Graphics& g,const Slot& slot,std::uint16_t sources) {
    using namespace Gdiplus;
    RectF bounds(float(slot.x*kScale),float(slot.y*kScale),float(slot.w*kScale),float(slot.h*kScale));
    g.SetCompositingMode(CompositingModeSourceCopy);
    SolidBrush clear(Color(0,0,0,0));
    g.FillRectangle(&clear,bounds);
    g.SetCompositingMode(CompositingModeSourceOver);
    constexpr int order[]={14,13,15,12,8,9,10,11,3,0,1,2,4,5,6,7};
    std::vector<int> buttons; for(int i:order) if(sources&(1u<<i)) buttons.push_back(i);
    if(buttons.empty()) {
        Pen p(Color(230,190,197,208),std::max(2.0f,bounds.Height*.05f));
        g.DrawLine(&p,bounds.X+bounds.Width*.3f,bounds.Y+bounds.Height*.5f,bounds.X+bounds.Width*.7f,bounds.Y+bounds.Height*.5f);
        return;
    }
    // '+' in a mapping means alternatives (logical OR), never a chord.
    // Pack full-sized keycaps into this atlas cell. Expand restores their
    // aspect and size when drawing, rather than shrinking alternative labels.
    const auto [columns,rows]=Grid(sources);
    auto saved=g.Save(); g.ScaleTransform(1.0f/columns,1.0f/rows);
    for(std::size_t i=0;i<buttons.size();++i) {
        RectF cell(bounds.X*columns+(i%columns)*bounds.Width,bounds.Y*rows+(i/columns)*bounds.Height,
                   bounds.Width,bounds.Height);
        Button(g,cell,buttons[i]);
    }
    g.Restore(saved);
}
}

void Initialize() {
    config=controls::Config{};
    for(int i=0;i<16;++i) config.sources[i]=controls::ParseSources(Setting(std::string("map.")+controls::Destination[i],
        controls::Physical[i]),config.sources[i]);
    enabled=SettingOn("xbox_prompts",true); images.clear(); textures.clear();
    if(enabled) std::fprintf(stderr,"[Xbox prompts] generating images from the active button mappings\n");
}
bool Enabled() { return enabled; }
bool Matches(const texture_pack::Name& name) {
    return enabled && name.texture==0x6c432b9f02f261c7ull && (name.bits&0x3fffu)==0x19d3u;
}
void RegisterTexture(std::uint64_t key) { textures.insert(key); }
bool HasTexture(std::uint64_t key) { return enabled && textures.count(key); }
bool Expand(std::uint64_t key,ps2gpu::GsVertex* vertices,std::size_t count,
    std::uint32_t width,std::uint32_t height) {
    if(!enabled || !textures.count(key) || !count) return false;
    float u0=128,v0=64,u1=0,v1=0,x0=float(width),y0=float(height),x1=0,y1=0;
    for(std::size_t i=0;i<count;++i) {
        const auto& v=vertices[i]; if(v.q==0) return false;
        float u=v.s/v.q*128, t=v.t/v.q*64;
        u0=std::min(u0,u); u1=std::max(u1,u); v0=std::min(v0,t); v1=std::max(v1,t);
        x0=std::min(x0,v.x); x1=std::max(x1,v.x); y0=std::min(y0,v.y); y1=std::max(y1,v.y);
    }
    for(const auto& slot:slots) {
        if(u0<slot.x-.6f || u1>slot.x+slot.w+.6f || v0<slot.y-.6f || v1>slot.y+slot.h+.6f) continue;
        const auto [columns,rows]=Grid(config.sources[slot.destination]);
        constexpr float legibility=1.8f;
        const float sx=columns*legibility,sy=rows*legibility;
        const float w=(x1-x0)*sx,h=(y1-y0)*sy;
        float left=x0<float(width)*.35f ? x1-w : x0;
        left=std::clamp(left,0.0f,std::max(0.0f,float(width)-w));
        float top=std::max(0.0f,(y0+y1-h)*.5f);
        for(std::size_t i=0;i<count;++i) {
            vertices[i].x=left+(vertices[i].x-x0)*sx;
            vertices[i].y=top+(vertices[i].y-y0)*sy;
        }
        return true;
    }
    return false;
}
std::shared_ptr<const texture_pack::Image> Cached(const texture_pack::Name& name) {
    auto found=images.find(name.Text()); return found==images.end()?nullptr:found->second;
}
std::shared_ptr<texture_pack::Image> Build(const controls::Config& mapping,
    const std::vector<std::uint32_t>& original,std::uint32_t width,std::uint32_t height) {
    if(width!=128 || height!=64 || original.size()!=width*height) return nullptr;
    static Gdi gdiplus;
    if(!gdiplus.token) return nullptr;
    using namespace Gdiplus;
    std::vector<std::uint32_t> bgra(kWidth*kHeight);
    for(int y=0;y<kHeight;++y) for(int x=0;x<kWidth;++x) {
        auto rgba=original[(y/kScale)*128+x/kScale];
        auto alpha=std::min(255u,(rgba>>24)*255u/128u);
        bgra[y*kWidth+x]=(rgba&0xff00u)|((rgba&0xffu)<<16)|((rgba>>16)&0xffu)|(alpha<<24);
    }
    Bitmap bitmap(kWidth,kHeight,kWidth*4,PixelFormat32bppARGB,reinterpret_cast<BYTE*>(bgra.data()));
    {
        Graphics g(&bitmap); g.SetSmoothingMode(SmoothingModeAntiAlias); g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
        g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
        // This empty circle was a decorative surround drawn behind each hint.
        // Modern keycaps already have a border and are drawn at a readable size.
        g.SetCompositingMode(CompositingModeSourceCopy);
        SolidBrush clear(Color(0,0,0,0));
        g.FillRectangle(&clear,0,0,29*kScale,30*kScale);
        g.FillRectangle(&clear,0,32*kScale,27*kScale,24*kScale);
        g.SetCompositingMode(CompositingModeSourceOver);
        for(const auto& slot:slots) Alternatives(g,slot,mapping.sources[slot.destination]);
        g.Flush(FlushIntentionSync);
    }
    auto image=std::make_shared<texture_pack::Image>(); image->width=kWidth; image->height=kHeight;
    image->format=texture_pack::kRgba8; image->levels=1; image->data.resize(bgra.size()*4);
    for(std::size_t i=0;i<bgra.size();++i) {
        auto c=bgra[i]; auto a=std::min(128u,((c>>24)*128u+127u)/255u);
        auto rgba=(c&0xff00u)|((c&0xffu)<<16)|((c>>16)&0xffu)|(a<<24);
        std::memcpy(image->data.data()+i*4,&rgba,4);
    }
    return image;
}
std::shared_ptr<const texture_pack::Image> Make(const texture_pack::Name& name,
    const std::vector<std::uint32_t>& original,std::uint32_t width,std::uint32_t height) {
    auto image=Build(config,original,width,height);
    if(image) {
        image->id=next_id++; images[name.Text()]=image;
        std::fprintf(stderr,"[Xbox prompts] generated atlas %ux%u, %s\n",image->width,image->height,name.Text().c_str());
    }
    return image;
}
}
