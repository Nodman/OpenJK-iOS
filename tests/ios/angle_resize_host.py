#!/usr/bin/env python3
"""Exercise app resize ordering against the pinned Metal drawable state machine."""
import argparse
from pathlib import Path
import subprocess
import tempfile

PIN = '3c3c1746733c752cfe8cf1ff9c2524885d5bd28e'
parser = argparse.ArgumentParser()
parser.add_argument('--angle-source', type=Path, required=True)
parser.add_argument('--output', type=Path)
parser.add_argument('--splash-root', type=Path, default=Path(__file__).resolve().parents[2])
parser.add_argument('--gl4es-source', type=Path, required=True)
parser.add_argument('--unflushed-control', action='store_true')
parser.add_argument('--app-source', type=Path, default=Path(__file__).resolve().parents[2] / 'shared/sdl/sdl_window.cpp')
args = parser.parse_args()
rel = 'src/libANGLE/renderer/metal/SurfaceMtl.mm'
backend = (args.angle_source / rel).read_text()
assert subprocess.check_output(['git', '-C', str(args.angle_source), 'show', f'{PIN}:{rel}']).decode() == backend
app = args.app_source.read_text()


def function(source, marker):
    start = source.index(marker)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


methods = '\n'.join(function(backend, marker) for marker in (
    'CGSize WindowSurfaceMtl::calcExpectedDrawableSize()',
    'bool WindowSurfaceMtl::checkIfLayerResized(',
    'angle::Result WindowSurfaceMtl::ensureCurrentDrawableObtained(',
    'angle::Result WindowSurfaceMtl::swapImpl('))
# Mechanical ObjC/property adaptations; the branches and state changes stay exact.
methods = methods.replace('mMetalLayer.get().', 'mMetalLayer.get()->')
methods = methods.replace('mLayer.contentsScale', 'mLayer->contentsScale').replace('mLayer.bounds', 'mLayer->bounds')
methods = methods.replace('nil', '0')
start = app.index('\tconst float renderScale = GLimp_IOS_RenderScale();') if '\tconst float renderScale = GLimp_IOS_RenderScale();' in app else app.index('\tSys_IOS_SetDrawableScale( screen, GLimp_IOS_RenderScale() );')
end = app.index('\n\tSys_IOS_GetDrawableSize( screen, &glConfig->vidWidth', start)
resize = app[start:end]
prefix = r'''
#include "qcommon/ios_render_scale.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <initializer_list>
#undef TARGET_OS_OSX
#define TARGET_OS_OSX 0
#define TARGET_OS_MACCATALYST 0
#define ASSERT assert
#define ANGLE_TRY(x) do { if ((x) != angle::Result::Continue) return angle::Result::Stop; } while (0)
namespace angle { enum class Result { Continue, Stop }; }
namespace gl { struct Context {}; }
using CGFloat = double;
struct CGSize { double width, height; };
struct CGRect { CGSize size; };
CGSize CGSizeMake(double w, double h) { return {w,h}; }
bool CGRectEqualToRect(CGRect a, CGRect b) { return a.size.width==b.size.width && a.size.height==b.size.height; }
struct Layer { CGRect bounds{{956,440}}; double contentsScale=3; CGSize drawableSize{2868,1320}; } layer;
struct Texture { void set(void*) {} } texture;
struct ContextMtl { void present(const gl::Context*, int) {} } ctx;
namespace mtl { ContextMtl* GetImpl(const gl::Context*) { return &ctx; } }
void StopFrameCapture() {} void StartFrameCapture(ContextMtl*) {}
struct Holder { Layer* get() const { return &layer; } operator Layer*() const { return &layer; } };
struct WindowSurfaceMtl {
 Holder mMetalLayer; Layer* mLayer=&layer; CGSize mCurrentKnownDrawableSize{2868,1320};
 int mCurrentDrawable=0; Texture* mColorTexture=&texture; bool mMSColorTexture=false;
 bool mDepthTexture=true, mStencilTexture=true, mDepthStencilTexturesInitialized=true;
 int acquisitions=0, resizes=0;
 CGSize calcExpectedDrawableSize() const;
 bool checkIfLayerResized(const gl::Context*);
 angle::Result ensureCurrentDrawableObtained(const gl::Context*);
 angle::Result swapImpl(const gl::Context*);
 angle::Result resolveColorTextureIfNeeded(const gl::Context*) { return angle::Result::Continue; }
 angle::Result obtainNextDrawable(const gl::Context* c) {
   if (checkIfLayerResized(c)) ++resizes;
   ++acquisitions; mCurrentDrawable=1; return angle::Result::Continue;
 }
} surface;
'''
suffix = r'''
bool angleActive=true, failPresent=false;
int screen=1, swaps=0, fatals=0;
float nativeScale=3, requestedScale=1, viewScale=3;
int maxfps=120;
constexpr int ERR_FATAL=1, RSERR_UNKNOWN=1;
float GLimp_IOS_RenderScale() { return IOS_RenderScale_Resolve(requestedScale, 1, IOS_RenderScale_HighRefresh(maxfps), 2868, 1320).scale; }
float Sys_IOS_GetDrawableScale(int) { return viewScale; }
bool Sys_IOS_EGL_Present() {
 ++swaps; if (failPresent) return false;
 return surface.swapImpl(nullptr)==angle::Result::Continue;
}
void Com_Error(int, const char*) { ++fatals; throw std::runtime_error("fatal"); }
void Sys_IOS_SetDrawableScale(int, float s) {
 viewScale=nativeScale*s; layer.contentsScale=viewScale;
 layer.drawableSize=CGSizeMake(layer.bounds.size.width*viewScale,layer.bounds.size.height*viewScale);
}
int resizeApp() {
'''
main = r'''
 return 0;
}
bool equal(CGSize a, CGSize b) { return int(a.width)==int(b.width) && int(a.height)==int(b.height); }
void frame() { assert(surface.ensureCurrentDrawableObtained(nullptr)==angle::Result::Continue); }
int main() {
 // Initial attachment has no drawable. First rendering observes the new scale.
 requestedScale=0; assert(resizeApp()==0);
 assert(!equal(surface.mCurrentKnownDrawableSize,layer.drawableSize));
 frame(); surface.swapImpl(nullptr);
 assert(equal(surface.mCurrentKnownDrawableSize,layer.drawableSize));
 // Shutdown/initialization work may acquire a drawable before the scale edit.
 // Same-scale restarts must not introduce an extra present.
 for(int i=0;i<3;++i) {
  frame(); int oldSwaps=swaps; assert(resizeApp()==0); assert(swaps==oldSwaps);
  frame(); surface.swapImpl(nullptr); assert(equal(surface.mCurrentKnownDrawableSize,layer.drawableSize));
 }
 // The reproduced .8 -> .5 transition, then native, auto, and repeated cycles.
 for(float scale : {.5f,1.f,0.f,.5f,1.f,0.f}) {
  frame(); int oldSwaps=swaps; requestedScale=scale; assert(resizeApp()==0);
  frame(); surface.swapImpl(nullptr);
  printf("scale %.1f: SDL=%dx%d EGL=%dx%d\n",scale,int(layer.drawableSize.width),int(layer.drawableSize.height),int(surface.mCurrentKnownDrawableSize.width),int(surface.mCurrentKnownDrawableSize.height));
  fflush(stdout);
  assert(equal(surface.mCurrentKnownDrawableSize,layer.drawableSize));
  assert(swaps==oldSwaps+1); assert(!surface.mCurrentDrawable);
 }
 // No in-flight drawable: retirement is a harmless swap, then size converges.
 requestedScale=.5f; assert(!surface.mCurrentDrawable); resizeApp(); frame(); surface.swapImpl(nullptr);
 assert(equal(surface.mCurrentKnownDrawableSize,layer.drawableSize));
 // Auto uses native dimensions at 60 and 0.8 native at 120.
 requestedScale=0;
 for(int rate : {60,120,60,120}) {
  frame(); maxfps=rate; int oldSwaps=swaps; resizeApp(); frame(); surface.swapImpl(nullptr);
  assert(swaps==oldSwaps+1 && equal(surface.mCurrentKnownDrawableSize,layer.drawableSize));
  assert(int(layer.drawableSize.width)==(rate==60 ? 2868 : 2294));
  assert(int(layer.drawableSize.height)==(rate==60 ? 1320 : 1056));
 }
 // EAGL and sub-threshold changes never use the EGL present path.
 angleActive=false; requestedScale=1.f; int oldSwaps=swaps; resizeApp(); assert(swaps==oldSwaps);
 angleActive=true; requestedScale=.9999f; resizeApp(); assert(swaps==oldSwaps);
 // Present failure is fatal before mutating the layer or scale.
 requestedScale=.5f; failPresent=true; auto before=layer.drawableSize; float oldScale=viewScale;
 try { resizeApp(); assert(false); } catch(const std::runtime_error&) {}
 assert(fatals==1 && viewScale==oldScale && equal(before,layer.drawableSize));
 printf("PASS: startup, three same-scale restarts, 0.5/native/auto cycles, empty drawable, auto60/120 transitions, EAGL, tolerance, fatal ordering\n");
}
'''
code = prefix + methods + suffix + resize + main
out = args.output or Path(tempfile.mkdtemp(prefix='openjk-angle-resize-'))
out.mkdir(parents=True, exist_ok=True)
(out / 'angle_resize.cc').write_text(code)
subprocess.run(['clang++', '-std=c++17', '-Wall', '-Wextra', '-Wno-unused-parameter', '-I' + str(Path(__file__).resolve().parents[2] / 'shared'), str(out / 'angle_resize.cc'), '-o', str(out / 'angle_resize')], check=True)
subprocess.run([str(out / 'angle_resize')], check=True)

# Execute complete production splash bodies, preserving their GLES guards/branches.
# gl4es flush body/macro are extracted too; only CPU drain/native GPU work are modeled.
gl4es = (args.gl4es_source / 'src/gl/gl4es.c').read_text()
flush = function(gl4es, 'void APIENTRY_GL4ES gl4es_glFlush(void)')
flush_macro = next(line for line in (args.gl4es_source / 'src/gl/gl4es.h').read_text().splitlines()
                   if line.startswith('#define FLUSH_BEGINEND '))
splash_stubs = r'''
#include <vector>
#include <algorithm>
#define APIENTRY_GL4ES
#define NOX11
struct { struct { bool pending=false; } list; struct { bool bm_drawing=false; } raster; } state;
auto* glstate=&state;
std::vector<char> events;
int drains=0, flushes=0, nativeFlushes=0, presents=0, clears=0;
bool imageAvailable=true, pendingAtPresent=false, matchedAtPresent=false;
void nativeFlush() { ++nativeFlushes; events.push_back('N'); }
#define LOAD_GLES(name) auto gles_##name = nativeFlush
void realize_textures(int) { ++flushes; events.push_back('F'); }
void bitmap_flush() { assert(false); }
void errorGL() {}
void gl4es_flush() {
 assert(glstate->list.pending); ++drains; events.push_back('D');
 frame(); glstate->list.pending=false;
}
struct image_t {} image;
constexpr int qfalse=0, GL_CLAMP=1, GLS_SRCBLEND_ONE=2, GLS_DSTBLEND_ZERO=4;
constexpr int GL_TRIANGLE_STRIP=5, GL_COLOR_BUFFER_BIT=6;
image_t* R_FindImageFile(const char*,int,int,int,int) { return imageAvailable ? &image : nullptr; }
void RB_SetGL2D() {} void GL_Bind(image_t*) {} void GL_State(int) {}
void qglBegin(int) {} void qglTexCoord2f(float,float) {} void qglVertex2f(float,float) {}
void qglEnd() { events.push_back('E'); glstate->list.pending=true; }
void qglClearColor(float,float,float,float) {}
void qglClear(int) { ++clears; events.push_back('C'); frame(); }
int window=0;
struct Imports {
 void WIN_Present(int*) {
  ++presents; events.push_back('P'); pendingAtPresent=glstate->list.pending;
  surface.swapImpl(nullptr); // No unconditional acquisition on presentation.
  matchedAtPresent=equal(surface.mCurrentKnownDrawableSize,layer.drawableSize);
 }
} ri;
'''
splash_main = r'''
void reset() {
 surface=WindowSurfaceMtl{}; layer=Layer{}; state={};
 angleActive=true; failPresent=false; nativeScale=3; requestedScale=0;
 viewScale=3; maxfps=120; swaps=0; fatals=0;
 events.clear(); drains=flushes=nativeFlushes=presents=clears=0;
}
void runSplash(bool nativeOnly=false) {
 events.clear(); int oldFlushes=flushes, oldDrains=drains, oldPresents=presents;
 qglFlush=nativeOnly ? nativeFlush : gl4es_glFlush;
 R_Splash();
 assert(presents==oldPresents+1 && events.back()=='P');
 bool wrapper=EXPECT_FLUSH && GLES_TEST && !nativeOnly;
 assert(flushes==oldFlushes+int(wrapper));
 if(wrapper) {
  auto f=std::find(events.begin(),events.end(),'F');
  assert(f!=events.end() && f!=events.begin() && (*(f-1)=='E' || *(f-1)=='C'));
  assert(!pendingAtPresent);
  assert(drains==oldDrains+int(!SP_TEST || imageAvailable));
 } else assert(drains==oldDrains);
}
int main() {
 for(bool hasImage : {true,false}) {
  imageAvailable=hasImage;
  for(bool nativeOnly : {false,true}) {
   reset(); resizeApp();
   // The startup-only implementation-read path acquires after layout.
   frame(); runSplash(nativeOnly); assert(matchedAtPresent);
   puts("PASS startup-only read path converges");
   for(int i=0;i<3;++i) {
    // Prior frame/shutdown is separate from splash and occurs before SetMode.
    if(state.list.pending) gl4es_flush();
    frame(); int oldSwaps=swaps; resizeApp(); assert(swaps==oldSwaps);
    runSplash(nativeOnly); assert(matchedAtPresent);
   }
   puts("PASS three same-scale splash restarts");
   if(state.list.pending) gl4es_flush();
   frame(); int oldSwaps=swaps; requestedScale=.5f; resizeApp();
   assert(swaps==oldSwaps+1 && !surface.mCurrentDrawable);
   runSplash(nativeOnly);
   bool queued=(!SP_TEST || imageAvailable) && !(EXPECT_FLUSH && GLES_TEST && !nativeOnly);
   assert(pendingAtPresent==queued && matchedAtPresent==!queued);
   assert(int(layer.drawableSize.width)==1434 && int(layer.drawableSize.height)==660);
   printf("half-scale image=%d native-only=%d: SDL1434x660 EGL%dx%d pending=%d\n",
          hasImage,nativeOnly,int(surface.mCurrentKnownDrawableSize.width),
          int(surface.mCurrentKnownDrawableSize.height),pendingAtPresent);
   if(queued) {
    assert(int(surface.mCurrentKnownDrawableSize.width)==2294);
    assert(int(surface.mCurrentKnownDrawableSize.height)==1056);
    // Native flush alone cannot drain the pending CPU list.
    nativeFlush(); assert(state.list.pending);
    gl4es_glFlush(); assert(!state.list.pending);
    surface.swapImpl(nullptr); assert(equal(surface.mCurrentKnownDrawableSize,layer.drawableSize));
   }
   if(EXPECT_FLUSH && GLES_TEST && !nativeOnly) {
    for(float scale : {1.f,0.f,.5f}) {
     frame(); requestedScale=scale; resizeApp(); runSplash(); assert(matchedAtPresent);
    }
   }
   assert(clears==((SP_TEST && !hasImage) ? presents : 0));
  }
 }
 puts("PASS extracted splash branches/guard/order, queue/native-only controls and return scales; GPU/UIKit unmodeled");
}
'''
for variant, rel in [('MP', 'codemp'), ('SP', 'code')]:
    splash = function((args.splash_root / rel / 'rd-vanilla/tr_init.cpp').read_text(), 'void R_Splash()')
    for gles in [False, True]:
        label = variant + ('-gles' if gles else '-desktop')
        source = prefix + methods + suffix + resize + main[:main.index('int main()')]
        source += splash_stubs + flush_macro + '\n' + flush
        source += '\nvoid (*qglFlush)()=gl4es_glFlush;\n' + splash + splash_main
        cc = out / (label + '.cc')
        cc.write_text(source)
        command = ['clang++', '-std=c++17', '-Wall', '-Wextra', '-Wno-unused-parameter',
                   '-I' + str(Path(__file__).resolve().parents[2] / 'shared'),
                   '-DGLES_TEST=' + str(int(gles)), '-DSP_TEST=' + str(int(variant == 'SP')),
                   '-DEXPECT_FLUSH=' + str(int(not args.unflushed_control))]
        if gles:
            command += ['-DOPENJK_GLES']
        subprocess.run(command + [str(cc), '-o', str(out / label)], check=True)
        print(label, flush=True)
        subprocess.run([str(out / label)], check=True)
