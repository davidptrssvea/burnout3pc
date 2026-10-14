#include <cstdio>
#include <vector>
#include "ps2gpu/gs_renderer.h"
void Quad(ps2gpu::GsRenderer& r,const ps2gpu::GsDrawState& s,float umax,float vmin,float vmax) {
    const float w=float(s.width),h=float(s.height);
    ps2gpu::GsVertex v[6]={{0,0,0,128,128,128,128,0,vmin,1},{w,0,0,128,128,128,128,umax,vmin,1},
      {0,h,0,128,128,128,128,0,vmax,1},{w,0,0,128,128,128,128,umax,vmin,1},
      {w,h,0,128,128,128,128,umax,vmax,1},{0,h,0,128,128,128,128,0,vmax,1}};
    r.DrawTriangles(s,v,6);
}
int main(int argc,char** argv) {
    std::string error;auto gpu=ps2gpu::CreateGpu(&error);if(!gpu) return 1;
    auto r=ps2gpu::CreateGsRenderer(*gpu,&error);if(!r) return 1;
    std::vector<std::uint32_t> tex(256*64);
    for(int y=0;y<64;++y)for(int x=0;x<256;++x)
        tex[y*256+x]=((x*37+y*19)&255)|(((x*13+y*43)&255)<<8)|(((x*7+y*29)&255)<<16)|(128u<<24);
    r->UploadTexture(1,tex.data(),256,64);
    ps2gpu::GsDrawState s;s.fbp=0x100;s.fbw=5;s.width=320;s.height=224;s.scissor_x1=319;s.scissor_y1=223;
    s.textured=true;s.texture=1;s.tfx=ps2gpu::TextureFunction::Decal;s.wms=s.wmt=0;
    r->SetScale(4);
    for(auto af:{1u,4u,8u,16u}) {
        s.anisotropy=af;double best=1e9;
        for(int pass=0;pass<5;++pass) {
            auto before=ps2gpu::GpuBusyNanoseconds();
            for(int i=0;i<24;++i) Quad(*r,s,32,.01f,.3f);
            std::vector<std::uint32_t> out;std::uint32_t w,h;r->ReadTargetFull(s.fbp,s.fbw,0,&out,&w,&h);
            auto after=ps2gpu::GpuBusyNanoseconds();
            if(pass>0) best=std::min(best,double(after-before)/1e6);
        }
        std::printf("AF %ux: %.3f GPU ms / 24 layers at 1280x896\n",af,best);
    }
    s.anisotropy=1;s.bilinear=true;
    for(auto target:{false,true}) {
        if(target){
            r->SetScale(2);s.width=640;s.height=448;s.fbw=10;s.scissor_x1=639;s.scissor_y1=447;
            std::vector<std::uint32_t> source(640*448,0x80634321);r->UploadTarget(0x300,10,0,source.data(),640,448);
            s.texture_target=true;s.tex_fbp=0x300;s.tex_fbw=10;s.tw=1024;s.th=512;
        }
        double best=1e9;
        for(int pass=0;pass<5;++pass){
            auto before=ps2gpu::GpuBusyNanoseconds();for(int i=0;i<24;++i)Quad(*r,s,target?.625f:32,.001f,target?.876f:.3f);
            std::vector<std::uint32_t> out;std::uint32_t w,h;r->ReadTargetFull(s.fbp,s.fbw,0,&out,&w,&h);
            auto after=ps2gpu::GpuBusyNanoseconds();if(pass>0)best=std::min(best,double(after-before)/1e6);
        }
        std::printf("%s: %.3f GPU ms / 24 layers at 1280x896\n",target?"Scaled post-effect":"GS bilinear",best);
    }
    s.texture_target=false;s.tw=s.th=0;s.bilinear=false;
    // Capture independent rows with repeat, edge clamp, region clamp/repeat,
    // magnification, minification, and a mip atlas whose levels have distinct colours.
    r->SetScale(1);s.fbp=0x120;s.fbw=1;s.width=s.height=64;s.scissor_x1=s.scissor_y1=63;
    auto file=argc==2?std::fopen(argv[1],"wb"):nullptr;
    for(auto af:{1u,4u,8u,16u})for(unsigned mode=0;mode<4;++mode)for(float span:{.15f,1.0f,4.0f}) {
        s.anisotropy=af;s.wms=s.wmt=mode;s.minu=3;s.maxu=31;s.minv=2;s.maxv=15;
        Quad(*r,s,span,-.1f,.4f);
        std::vector<std::uint32_t> out;std::uint32_t w,h;r->ReadTarget(s.fbp,s.fbw,0,&out,&w,&h);
        if(file)std::fwrite(out.data(),4,out.size(),file);
    }
    std::vector<std::uint32_t> mip(128*224);
    for(int y=0;y<224;++y)for(int x=0;x<128;++x) mip[y*128+x]=(y<128?0x800000FFu:y<192?0x8000FF00u:0x80FF0000u);
    r->UploadTexture(2,mip.data(),128,224);s.texture=2;s.tw=s.th=128;s.mip=2u|(2u<<2)|(5u<<5)|(1u<<8);
    for(unsigned mode=0;mode<4;++mode)for(float span:{1.0f,4.0f,8.0f}) {
        s.anisotropy=8;s.wms=s.wmt=mode;Quad(*r,s,span,-.1f,span);
        std::vector<std::uint32_t> out;std::uint32_t w,h;r->ReadTarget(s.fbp,s.fbw,0,&out,&w,&h);
        if(file)std::fwrite(out.data(),4,out.size(),file);
    }

    // GS 16-bit transfers: complete paired columns and partial columns,
    // nonzero first rows, both colour layouts and both depth layouts.
    for(auto psm:{2u,10u,50u,58u})for(auto cols:{1u,7u,8u,9u,15u,16u,63u,64u})for(auto first:{0u,7u,31u}) {
        r->SetScale(2);std::vector<std::uint8_t> init(8192,0xA5);r->MemoryWrite(12288,12320,init.data());
        std::vector<std::uint32_t> source(64*64);
        for(unsigned y=0;y<64;++y)for(unsigned x=0;x<64;++x)
            source[y*64+x]=psm<48 ? ((x*31+y*7)&255)|(((x*11+y*29)&255)<<8)|(((x*47+y*13)&255)<<16)|((x%3?128u:64u)<<24): (x+y*64)*7;
        if(psm<48){r->UploadTarget(0x180,1,psm,source.data(),64,64);r->TargetToMemory(0x180,1,psm,cols,50,first);}
        else {r->UploadDepth(0x180,64,64,source.data());r->DepthToMemory(0x180,64,64,1,psm,cols,50,first);}
        r->MemoryRead(12288,12320,init.data());if(file)std::fwrite(init.data(),1,init.size(),file);
    }
    std::vector<std::uint32_t> transfer(640*448,0x80402010);r->SetScale(2);r->UploadTarget(0x20,10,2,transfer.data(),640,448);
    double best=1e9;
    for(int pass=0;pass<5;++pass){
        auto before=ps2gpu::GpuBusyNanoseconds();for(int i=0;i<48;++i)r->TargetToMemory(0x20,10,2,640,448);
        std::vector<std::uint8_t> sync(256);r->MemoryRead(1024,1025,sync.data());auto after=ps2gpu::GpuBusyNanoseconds();
        if(pass>0)best=std::min(best,double(after-before)/1e6);
    }
    std::printf("16-bit transfer: %.3f GPU ms / 48 complete scaled buffers\n",best);
    if(file)std::fclose(file);
    return 0;
}
