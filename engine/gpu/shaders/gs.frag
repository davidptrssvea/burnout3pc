#version 450
// The GS's per-pixel work, in its own terms (0-255, with 0x80 as 1.0 where it
// multiplies): the texture function, then the alpha test. Blending is the
// fixed-function blender's; the GS's factor - source alpha or FIX over 128 -
// goes out as the second output (dual-source blending), since the alpha
// written is the source alpha as it is. Compiled a second time with
// SINGLE_OUTPUT for devices without dual-source blending, and a third with
// SHADER_BLEND: the GS's blend done here, from a copy of the target - for a
// factor the blender cannot have (FIX over 0x80: more than 1.0, which a
// fixed-point target clamps to 1).
#ifdef SHADER_BLEND
#define SINGLE_OUTPUT
layout(set = 2, binding = 0) uniform sampler2D destination;
#endif

layout(location = 0) in vec4 shade;
layout(location = 1) noperspective in vec3 coordinate;
layout(location = 2) noperspective in float fog;
layout(location = 3) flat in vec4 range;   // a sprite's S, T at its first and last pixels (gs.vert)

layout(set = 0, binding = 0) uniform sampler2D texture_image;
// The GPU's CLUT (decode.comp's), 256 entries as the GS expands them: a target
// read as PSMT8H (tex_format 4) indexes it by its alpha byte.
layout(std430, set = 1, binding = 0) readonly buffer Clut { uint clut[256]; };

layout(push_constant) uniform Params {
    layout(offset = 16) uint textured;
    uint tfx;           // 0 MODULATE, 1 DECAL, 2 HIGHLIGHT, 3 HIGHLIGHT2
    uint tcc;           // the texel's alpha counts
    uint atst;          // 0 NEVER, 1 ALWAYS, 2 LESS, 3 LEQUAL, 4 EQUAL, 5 GEQUAL, 6 GREATER, 7 NOTEQUAL
    uint aref;
    uint fix_factor;    // blend with FIX rather than the source alpha
    uint fix;
    uint afail;         // which pixels this pass draws: 0 passing, 1 failing, 2 all
    uint fge;           // fog
    uint fogcol;        // FOGCOL, R G B in bytes 0-2
    uint pabe;          // which pixels this pass draws: 0 all, 1 As >= 0x80, 2 As < 0x80
    uint bilinear;      // bit 0: the four texels around, else the nearest; bits 1 and on: Mip
    uint wms, wmt;      // CLAMP: 0 REPEAT, 1 CLAMP, 2 REGION_CLAMP, 3 REGION_REPEAT
    uint minu, maxu, minv, maxv;
    uint tw, th;        // TEX0's size (0: the image's own)
    uint tex_format;    // a target read as 0 32-bit, 1 24-bit, 2 16-bit texels, 4 PSMT8H through the CLUT
    uint ta0, ta1, aem; // TEXA
    uint logic;         // FBMSK masking part of a channel: 1 the AND pass, 2 the OR pass, 0 neither
    uint keep;          // ...the bits FBMSK keeps, RGBA bytes as the target holds them
    uint scale;         // a target drawn larger (the internal resolution): its pixels to a GS texel
    uint blend_ops;     // bits 0-7, SHADER_BLEND: ALPHA's A, B, C, D, two bits each; 8-15: Snap's scale;
                        // 16: a 16-bit target (five bits a channel stored)
} p;

// A texel coordinate wrapped as CLAMP says, as the software GS's Wrap. A
// target drawn larger (Fine) is addressed in its own pixels: its size and a
// region's bounds scaled; REGION_REPEAT's masks do not scale (Texel takes
// those in GS texels).
int Wrap(int i, int size, uint mode, uint lo, uint hi, int k) {
    if (mode == 0u) return k == 1 ? (i & (size - 1)) : ((i % size) + size) % size;
    if (mode == 1u) return clamp(i, 0, size - 1);
    if (mode == 2u) return clamp(i, int(lo) * k, int(hi) * k + k - 1);
    return (i & int(lo)) | int(hi);
}

// Whether the texture is a target drawn larger, addressed in its own pixels.
bool Fine() { return p.scale > 1u && p.wms != 3u && p.wmt != 3u; }

ivec2 TextureSize() {
    return p.tw != 0u ? ivec2(int(p.tw), int(p.th)) : textureSize(texture_image, 0);
}

// A texel as the GS expands it: TEXA's alpha for 24- and 16-bit texels, a
// target read as PSMT8H through the CLUT.
vec4 Expand(vec4 t) {
    if (p.tex_format == 4u) {                       // PSMT8H: the alpha byte indexes the CLUT
        const uint c = clut[uint(t.a) & 255u];
        return vec4(float(c & 255u), float((c >> 8) & 255u), float((c >> 16) & 255u), float(c >> 24));
    }
    bool black = t.r + t.g + t.b == 0.0;
    if (p.tex_format == 1u) {                       // 24-bit: TEXA's alpha
        t.a = (p.aem != 0u && black) ? 0.0 : float(p.ta0);
    } else if (p.tex_format == 2u) {                // 16-bit: 5 bits a channel, alpha by its bit
        bool bit = t.a >= 128.0;
        t.rgb = floor(t.rgb / 8.0) * 8.0;
        black = t.r + t.g + t.b == 0.0;
        t.a = bit ? float(p.ta1) : ((p.aem != 0u && black) ? 0.0 : float(p.ta0));
    }
    return t;
}

vec4 Fetch(ivec2 at) { return Expand(floor(texelFetch(texture_image, at, 0) * 255.0 + 0.5)); }

vec4 Texel(int u, int v) {
    const int k = Fine() ? int(p.scale) : 1;
    ivec2 size = TextureSize() * k;
    ivec2 at = ivec2(Wrap(u, size.x, p.wms, p.minu, p.maxu, k), Wrap(v, size.y, p.wmt, p.minv, p.maxv, k));
    if (k == 1) at &= 2047;
    // A target drawn larger with REGION_REPEAT: the GS texel, and the middle
    // of its square.
    if (k == 1 && p.scale > 1u) at = at * int(p.scale) + int(p.scale / 2u);
    // A target may be smaller than TEX0 says: what lies beyond is not drawn here.
    at = min(at, textureSize(texture_image, 0) - 1);
    return Fetch(at);
}

// A texel of mipmap level n (n > 0: a texture from memory, never a target):
// TW-n by TH-n, below the levels before it at the image's left edge
// (gs_renderer.cpp's TextureFromMemory), a region's bounds shifted as its
// coordinates are.
vec4 Texel(int u, int v, int n) {
    if (n == 0) return Texel(u, v);
    const ivec2 size = max(TextureSize() >> n, ivec2(1));
    ivec2 at = ivec2(Wrap(u, size.x, p.wms, p.minu >> n, p.maxu >> n, 1),
                     Wrap(v, size.y, p.wmt, p.minv >> n, p.maxv >> n, 1));
    at = clamp(at, ivec2(0), size - 1);
    for (int i = 0; i < n; ++i) at.y += max(TextureSize().y >> i, 1);
    return Fetch(at);
}

// Address the four bilinear neighbours together. A screen-sized post effect
// otherwise repeats Fine/size/region/mip work four times, including division
// for a scaled target's REPEAT. Keep the exact GS weights and truncation.
ivec4 Neighbours(ivec2 i0,int n) {
    int k=n==0 && Fine()?int(p.scale):1;
    ivec2 size=n==0?TextureSize()*k:max(TextureSize()>>n,ivec2(1));
    ivec2 a=ivec2(Wrap(i0.x,size.x,p.wms,p.minu>>n,p.maxu>>n,k),Wrap(i0.y,size.y,p.wmt,p.minv>>n,p.maxv>>n,k));
    ivec2 b=ivec2(Wrap(i0.x+1,size.x,p.wms,p.minu>>n,p.maxu>>n,k),Wrap(i0.y+1,size.y,p.wmt,p.minv>>n,p.maxv>>n,k));
    if(n==0) {
        if(k==1) { a&=2047;b&=2047; }
        if(k==1 && p.scale>1u) { a=a*int(p.scale)+int(p.scale/2u);b=b*int(p.scale)+int(p.scale/2u); }
        a=min(a,textureSize(texture_image,0)-1);b=min(b,textureSize(texture_image,0)-1);
    } else {
        a=clamp(a,ivec2(0),size-1);b=clamp(b,ivec2(0),size-1);
        int offset=0;for(int i=0;i<n;++i) offset+=max(TextureSize().y>>i,1);
        a.y+=offset;b.y+=offset;
    }
    return ivec4(a.x,b.x,a.y,b.y);
}

// The texel at (u, v) in level 0's texels, in level n: the nearest, or the
// four around (u - 0.5, v - 0.5) mixed per channel as the GS mixes them: in
// sixteenths (its coordinates' 4 fractional bits), each step truncated - a
// game that filters its last frame back in relies on the loss (the software
// GS's Texel).
vec4 Sample(vec2 uv, int n, bool linear) {
    if (Fine()) uv *= float(p.scale);               // in the target's own pixels
    uv /= float(1 << n);
    if (!linear) return Texel(int(floor(uv.x)), int(floor(uv.y)), n);
    const ivec2 xy = ivec2(floor((uv - 0.5) * 16.0));
    const ivec2 i0 = xy >> 4;
    const vec4 f = vec4(xy & 15, 0, 0);
    const ivec4 at=Neighbours(i0,n);
    const vec4 c00=Fetch(at.xz),c10=Fetch(at.yz),c01=Fetch(at.xw),c11=Fetch(at.yw);
    const vec4 top = c00 + floor((c10 - c00) * f.x / 16.0), bottom = c01 + floor((c11 - c01) * f.x / 16.0);
    return top + floor((bottom - top) * f.y / 16.0);
}

// A texture pack's replacement (GsRenderer::UploadReplacement): finer than
// the GS's TW x TH, with mipmap levels of its own (levels - 1 in tex_format's
// bits 9-11), its texels RGBA as they are (alpha 0x80 is 1.0, as the GS's). Placed
// over the GS's texels: CLAMP's modes and region in them, k replacement
// texels to one; filtered as the game filters, with the level the pixel's
// footprint in the replacement's texels asks for (the GS's LOD knows only its
// own texels).
bool Replaced() { return (p.tex_format & 0x100u) != 0u; }

ivec2 ReplacedSize(int n) { return textureSize(texture_image, n); }

int ReplacedWrap(int i, int size, uint mode, uint lo, uint hi, float k) {
    if (mode == 0u) return ((i % size) + size) % size;
    if (mode == 1u) return clamp(i, 0, size - 1);
    if (mode == 2u) return clamp(i, int(floor(float(lo) * k)), int(ceil(float(hi + 1u) * k)) - 1);
    // REGION_REPEAT: the GS's texel masked, the replacement's texels within it kept.
    const int g = int(floor(float(i) / k));
    return int(float((g & int(lo)) | int(hi)) * k) + (i - int(float(g) * k));
}

vec4 ReplacedTexel(int u, int v, int n, vec2 k) {
    const ivec2 size = ReplacedSize(n);
    ivec2 at = ivec2(ReplacedWrap(u, size.x, p.wms, p.minu, p.maxu, k.x),
                     ReplacedWrap(v, size.y, p.wmt, p.minv, p.maxv, k.y));
    at = clamp(at, ivec2(0), size - 1);
    return floor(texelFetch(texture_image, at, n) * 255.0 + 0.5);
}

vec4 ReplacedLevel(vec2 r, int n, bool linear) {
    const vec2 k = vec2(ReplacedSize(n)) / vec2(TextureSize());      // its texels to a GS texel, at this level
    r /= float(1 << n);
    if (!linear) return ReplacedTexel(int(floor(r.x)), int(floor(r.y)), n, k);
    const vec2 xy = r - 0.5;
    const ivec2 i0 = ivec2(floor(xy));
    const vec2 f = xy - vec2(i0);
    const vec4 c00 = ReplacedTexel(i0.x, i0.y, n, k), c10 = ReplacedTexel(i0.x + 1, i0.y, n, k);
    const vec4 c01 = ReplacedTexel(i0.x, i0.y + 1, n, k), c11 = ReplacedTexel(i0.x + 1, i0.y + 1, n, k);
    return mix(mix(c00, c10, f.x), mix(c01, c11, f.x), f.y);
}

// `r`: where in level 0's texels; `footprint`: its derivatives' larger length.
vec4 ReplacedSample(vec2 r, float footprint) {
    const uint m = p.bilinear;
    const bool linear = (m & 2u) == 0u ? (m & 1u) != 0u : ((m >> 8) & 1u) != 0u || ((m >> 5) & 7u) >= 4u;
    const int last = int((p.tex_format >> 9) & 7u);
    const float lod = clamp(log2(max(footprint, 1e-6)), 0.0, float(last));
    const int n = int(floor(lod));
    const vec4 c0 = ReplacedLevel(r, n, linear);
    if (!linear || n >= last || lod == float(n)) return floor(c0 + 0.5);
    return floor(mix(c0, ReplacedLevel(r, n + 1, linear), lod - float(n)) + 0.5);
}

// TEX1's mipmapping (GsDrawState::mip, in p.bilinear): the GS's LOD from the
// pixel's Q - (log2(1/|Q|) << L) + K, or K alone (LCM) - magnifies (MMAG,
// level 0) up to 0, else minifies by MMIN: 2 and 4 the nearest level, 3 and 5
// the two around mixed; 4 and 5 bilinear within a level. The GS's own
// selection, not the GPU's: the levels it picks at any internal resolution
// (the software GS's ChooseLevel).
vec4 Sample(vec2 uv) {
    const uint m = p.bilinear;
    if ((m & 2u) == 0u) return Sample(uv, 0, (m & 1u) != 0u);
    const int mxl = int((m >> 2) & 7u);
    const uint mmin = (m >> 5) & 7u;
    float lod = float(int(m << 4) >> 20) / 16.0;    // K, 4 fractional bits
    const float q = coordinate.z != 0.0 ? abs(coordinate.z) : 1.0;     // as the software GS's
    if (((m >> 9) & 1u) == 0u) lod += -log2(q) * float(1u << ((m >> 10) & 3u));
    if (lod <= 0.0) return Sample(uv, 0, ((m >> 8) & 1u) != 0u);
    const bool linear = mmin == 4u || mmin == 5u;
    if (mmin == 2u || mmin == 4u) return Sample(uv, int(min(floor(lod + 0.5), float(mxl))), linear);
    lod = min(lod, float(mxl));
    const int n = int(floor(lod));
    const float f = lod - float(n);
    const vec4 c0 = Sample(uv, n, linear);
    if (n >= mxl || f == 0.0) return c0;
    const vec4 c1 = Sample(uv, n + 1, linear);
    return floor(c0 + (c1 - c0) * f + 0.5);
}

#ifdef SINGLE_OUTPUT
layout(location = 0) out vec4 colour;
#else
layout(location = 0, index = 0) out vec4 colour;
layout(location = 0, index = 1) out vec4 factor;
#endif

// The GS uses texelFetch and custom mip addressing: sampler anisotropy would
// have no effect here. Integrate the pixel's major footprint axis explicitly,
// selecting the mip by its minor axis. The host excludes HUD sprites and
// render-to-texture feedback from this optional filter.
// AF is an optional visual filter. Let the texture unit blend four neighbours
// in one lookup for decoded RGBA, while keeping GS wrapping and mip-atlas
// boundaries. Across a repeat seam the neighbours are not adjacent in the
// image, so use the original four-texel path there. Normal GS sampling keeps
// its exact two-stage truncation; only AF uses the hardware interpolation.
vec4 AnisotropicLevel(vec2 uv, int n, bool replacement) {
    if (!replacement && (p.tex_format != 0u || p.scale != 1u)) return Sample(uv,n,true);
    const ivec2 size=replacement ? ReplacedSize(n) : max(TextureSize()>>n,ivec2(1));
    const vec2 k=replacement ? vec2(size)/vec2(TextureSize()) : vec2(1.0);
    vec2 xy=uv/float(1<<n)-0.5;
    if(!replacement) xy=floor(xy*16.0)/16.0;
    const ivec2 i0=ivec2(floor(xy));
    vec2 fraction=xy-vec2(i0);
    ivec2 a,b;
    if(replacement) {
        a=ivec2(ReplacedWrap(i0.x,size.x,p.wms,p.minu,p.maxu,k.x),ReplacedWrap(i0.y,size.y,p.wmt,p.minv,p.maxv,k.y));
        b=ivec2(ReplacedWrap(i0.x+1,size.x,p.wms,p.minu,p.maxu,k.x),ReplacedWrap(i0.y+1,size.y,p.wmt,p.minv,p.maxv,k.y));
        a=clamp(a,ivec2(0),size-1);b=clamp(b,ivec2(0),size-1);
    } else {
        a=ivec2(Wrap(i0.x,size.x,p.wms,p.minu>>n,p.maxu>>n,1),Wrap(i0.y,size.y,p.wmt,p.minv>>n,p.maxv>>n,1));
        b=ivec2(Wrap(i0.x+1,size.x,p.wms,p.minu>>n,p.maxu>>n,1),Wrap(i0.y+1,size.y,p.wmt,p.minv>>n,p.maxv>>n,1));
        if(n==0) { a=min(a&2047,textureSize(texture_image,0)-1);b=min(b&2047,textureSize(texture_image,0)-1); }
        else { a=clamp(a,ivec2(0),size-1);b=clamp(b,ivec2(0),size-1); }
    }
    if((b.x!=a.x && b.x!=a.x+1)||(b.y!=a.y && b.y!=a.y+1))
        return replacement ? ReplacedLevel(uv,n,true) : Sample(uv,n,true);
    if(b.x==a.x) fraction.x=0.0;
    if(b.y==a.y) fraction.y=0.0;
    int offset=0;
    if(!replacement) for(int i=0;i<n;++i) offset+=max(TextureSize().y>>i,1);
    const vec2 at=vec2(a)+fraction+vec2(0.5,float(offset)+0.5);
    const int nativeLevel=replacement?n:0;
    return textureLod(texture_image,at/vec2(textureSize(texture_image,nativeLevel)),float(nativeLevel))*255.0;
}

vec4 AnisotropicSample(vec2 uv) {
    const bool replacement=Replaced();
    const vec2 ratio=replacement ? vec2(ReplacedSize(0))/vec2(TextureSize()) : vec2(1.0);
    const vec2 at=uv*ratio;
    const vec2 dx=dFdx(at), dy=dFdy(at);
    const float xx=dx.x*dx.x+dy.x*dy.x;
    const float yy=dx.y*dx.y+dy.y*dy.y;
    // This bound guarantees one tap at level zero, even for a rotated footprint.
    // Magnified textures need no eigenvector, normalisation or mip blending.
    if(xx+yy<=1.0) return floor(AnisotropicLevel(at,0,replacement)+0.5);
    const float xy=dx.x*dx.y+dy.x*dy.y;
    const float delta=sqrt(max((xx-yy)*(xx-yy)+4.0*xy*xy,0.0));
    const float major=sqrt(max((xx+yy+delta)*0.5,0.0));
    const float minor=sqrt(max((xx+yy-delta)*0.5,0.0));
    vec2 direction=vec2(xy,major*major-xx);
    if(dot(direction,direction)<1e-12) direction=xx>=yy ? vec2(1,0) : vec2(0,1);
    direction=normalize(direction);
    const int cap=1<<int(min((p.bilinear>>12)&15u,4u));
    const int taps=clamp(int(ceil(major/max(minor,1.0))),1,cap);
    const int last=replacement ? int((p.tex_format>>9)&7u) :
                    ((p.bilinear&2u)!=0u ? int((p.bilinear>>2)&7u) : 0);
    const float lod=clamp(log2(max(max(minor,major/float(taps)),1.0)),0.0,float(last));
    const int level=int(floor(lod));
    const int next=min(level+1,last);
    vec4 result=vec4(0);
    for(int i=0;i<taps;++i) {
        const vec2 sampleAt=at+direction*major*((float(i)+0.5)/float(taps)-0.5);
        const vec4 a=AnisotropicLevel(sampleAt,level,replacement);
        // Most game textures have no mip chain. Do not fetch the same level twice.
        if(next==level || lod==float(level)) result+=a;
        else result+=mix(a,AnisotropicLevel(sampleAt,next,replacement),lod-float(level));
    }
    return floor(result/float(taps)+0.5);
}

void main() {
    vec4 v = round(shade * 255.0);
    vec4 c = v;
    if (p.textured != 0u) {
        vec2 uv = coordinate.xy / coordinate.z * vec2(TextureSize());
        // A sprite on a target drawn larger: sampled as finely as the target
        // is drawn, within the coordinates the GS reads at its first and last
        // pixels (gs_renderer.cpp's DrawTriangles: further, where it meets
        // another), and - REPEAT, the sprite within one copy of the texture -
        // not wrapping round to its far side (a tile's edge: seams).
        const uint snap = (p.blend_ops >> 8) & 255u;
        if (snap > 1u) {
            const vec2 size = vec2(TextureSize());
            const vec2 lo = min(range.xy, range.zw), hi = max(range.xy, range.zw);
            uv = clamp(uv, lo * size, hi * size);
            const float inset = p.bilinear != 0u ? 0.5 : 0.001;
            if (p.wms == 0u && lo.x >= 0.0 && hi.x <= 1.0) uv.x = clamp(uv.x, inset, size.x - inset);
            if (p.wmt == 0u && lo.y >= 0.0 && hi.y <= 1.0) uv.y = clamp(uv.y, inset, size.y - inset);
        }
        // The footprint in a replacement's texels, from the coordinates before
        // the sprite's clamp above (whose edges would read as a large one).
        vec4 t;
        if (((p.bilinear>>12)&15u)!=0u) {
            t=AnisotropicSample(uv);
        } else if (Replaced()) {
            const vec2 k = vec2(ReplacedSize(0)) / vec2(TextureSize());
            const vec2 r = coordinate.xy / coordinate.z * vec2(TextureSize()) * k;
            const vec2 dx = dFdx(r), dy = dFdy(r);
            t = ReplacedSample(uv * k, sqrt(max(dot(dx, dx), dot(dy, dy))));
        } else {
            t = Sample(uv);
        }
        vec3 product = floor(t.rgb * v.rgb / 128.0);
        if (p.tfx == 0u) {
            c.rgb = product;
            c.a = p.tcc != 0u ? floor(t.a * v.a / 128.0) : v.a;
        } else if (p.tfx == 1u) {
            c.rgb = t.rgb;
            c.a = p.tcc != 0u ? t.a : v.a;
        } else if (p.tfx == 2u) {
            c.rgb = product + v.a;
            c.a = p.tcc != 0u ? t.a + v.a : v.a;
        } else {
            c.rgb = product + v.a;
            c.a = p.tcc != 0u ? t.a : v.a;
        }
        c = min(c, vec4(255.0));
    }

    if (p.fge != 0u) {
        // Towards FOGCOL as F falls, colour only.
        float f = round(fog * 255.0);
        vec3 fc = vec3(float(p.fogcol & 0xFFu), float((p.fogcol >> 8) & 0xFFu), float((p.fogcol >> 16) & 0xFFu));
        c.rgb = floor((f * c.rgb + (255.0 - f) * fc) / 256.0);
    }

    uint a = uint(c.a);
    bool pass = true;
    switch (p.atst) {
        case 0u: pass = false; break;
        case 2u: pass = a < p.aref; break;
        case 3u: pass = a <= p.aref; break;
        case 4u: pass = a == p.aref; break;
        case 5u: pass = a >= p.aref; break;
        case 6u: pass = a > p.aref; break;
        case 7u: pass = a != p.aref; break;
        default: break;
    }
    // The renderer draws AFAIL as passes: 0 keeps the passing pixels, 1 the
    // failing ones (with only what AFAIL writes), 2 all of them.
    if (p.afail == 0u && !pass) discard;
    if (p.afail == 1u && pass) discard;
    if (p.pabe == 1u && c.a < 128.0) discard;
    if (p.pabe == 2u && c.a >= 128.0) discard;

#ifdef SHADER_BLEND
    // Cv = ((A - B) * C >> 7) + D, clamped (COLCLAMP), as the software GS.
    vec4 dst = round(texelFetch(destination, ivec2(gl_FragCoord.xy), 0) * 255.0);
    uint ops = p.blend_ops & 255u;
    vec3 op_a = (ops & 3u) == 0u ? c.rgb : (ops & 3u) == 1u ? dst.rgb : vec3(0.0);
    vec3 op_b = ((ops >> 2) & 3u) == 0u ? c.rgb : ((ops >> 2) & 3u) == 1u ? dst.rgb : vec3(0.0);
    float op_c = ((ops >> 4) & 3u) == 0u ? c.a : ((ops >> 4) & 3u) == 1u ? dst.a : float(p.fix);
    vec3 op_d = ((ops >> 6) & 3u) == 0u ? c.rgb : ((ops >> 6) & 3u) == 1u ? dst.rgb : vec3(0.0);
    c.rgb = clamp(floor((op_a - op_b) * op_c / 128.0) + op_d, 0.0, 255.0);
#endif
    // A 16-bit target keeps the top five bits of each colour byte.
    if (((p.blend_ops >> 16) & 1u) != 0u) c.rgb = floor(c.rgb / 8.0) * 8.0;
    colour = c / 255.0;
    // A partly masked channel: the target ANDed with what FBMSK keeps, then
    // ORed with the rest of the source's bits (the pipelines' logic ops).
    if (p.logic == 1u) colour = unpackUnorm4x8(p.keep);
    if (p.logic == 2u) colour = unpackUnorm4x8(packUnorm4x8(c / 255.0) & ~p.keep);
#ifndef SINGLE_OUTPUT
    float k = p.fix_factor != 0u ? float(p.fix) : c.a;
    factor = vec4(k / 128.0);
#endif
}
