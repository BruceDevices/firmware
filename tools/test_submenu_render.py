"""Compile the actual submenu renderer against a fake display; no ESP32/PlatformIO required."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "src/core/display.cpp").read_text(encoding="utf-8")

def extract_renderer(source):
    start = source.index("void drawSubmenu(")
    return source[start:source.index("\nvoid drawStatusBar()", start)]

renderer = extract_renderer(SOURCE)
# Full redraw is the oracle for stale pixels left by incremental navigation.
reference = """
void drawSubmenuReference(int index, std::vector<Option>& options, const char* title) {
    drawSubmenu(index, options, title, true, -1);
}
"""
timer_start = SOURCE.index("        if ((menuType == MENU_TYPE_MAIN || menuType == MENU_TYPE_SUBMENU) &&")
timer_end = SOURCE.index("\n        }", timer_start) + len("\n        }")
timer = SOURCE[timer_start:timer_end]
init_start = SOURCE.index("    if (!compact) drawMainBorder();")
init_end = SOURCE.index("    while (1)", init_start)
init = SOURCE[init_start:init_end]
assert SOURCE.index("submenuNeedsFullRedraw = true;", SOURCE.index("bool renderedByLambda")) < SOURCE.index("renderedByLambda = options.renderHover(index)")
assert SOURCE.index("drawSubmenu(index, options.normalOptions(), subText,") < timer_start
assert SOURCE.index("statusBarDrawTs = millis();", init_start) < init_end

preamble = r"""
#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#ifndef FP
#define FP 1
#endif
#ifndef FM
#define FM 2
#endif
#ifndef FG
#define FG 3
#endif
#define LH 8
#define LW 6
#define BORDER_OFFSET_FROM_SCREEN_EDGE 5
#define BORDER_PAD_X (LW * FP + 4)
#define STATUS_BAR_HEIGHT (LH * FM + 14)
#define SMOOTH_FONT 1
#define TFT_DARKGREY 55
#define MENU_TYPE_MAIN 0
#define MENU_TYPE_SUBMENU 1
#define MENU_TYPE_REGULAR 2
int tftWidth = 320, tftHeight = 170;
struct { uint16_t priColor = 101, secColor = 202, bgColor = 0; } bruceConfig;
struct Option { std::string label; bool enabled = true; };
struct Rect { int x,y,w,h,color; };
struct Text { std::string text; int size,color; };
struct FakeTft {
    int size=FM, color=101;
    std::vector<int> pixels = std::vector<int>(tftWidth*tftHeight,0);
    std::vector<Rect> rects;
    std::vector<Text> texts;
    void setTextColor(int c, int = 0) { color=c; }
    void setTextSize(int n) { size=n; }
    void put(int x,int y,int c) { if(x>=0 && x<tftWidth && y>=0 && y<tftHeight) pixels[y*tftWidth+x]=c; }
    void paint(int x,int y,int w,int h,int c) { for(int dy=0;dy<h;++dy) for(int dx=0;dx<w;++dx) put(x+dx,y+dy,c); }
    void drawPixel(int x,int y,int c) { put(x,y,c); }
    void fillRect(int x,int y,int w,int h,int c) { rects.push_back({x,y,w,h,c}); paint(x,y,w,h,c); }
    void text(const std::string& s,int x,int y) {
        texts.push_back({s,size,color});
        for(size_t i=0;i<s.size();++i)
            for(int dy=0;dy<size*LH;++dy)
                for(int dx=0;dx<size*LW;++dx)
                    if((dx+dy+static_cast<unsigned char>(s[i]))%3 != 0) put(x+i*size*LW+dx,y+dy,color);
    }
    void drawString(const std::string& s,int x,int y,int = 1) { text(s,x,y); }
    void drawCentreString(const std::string& s,int x,int y,int = 1) { text(s,x-static_cast<int>(s.size())*size*LW/2,y); }
    void drawFastHLine(int x,int y,int w,int c) { paint(x,y,w,1,c); }
    void resetCalls() { rects.clear(); texts.clear(); }
};
FakeTft tft;
unsigned long fakeTime=0, drawDuration=0;
int statusReads=0, footerDraws=0;
bool submenuNeedsFullRedraw=false, redraw=false, overwriteStatusChrome=false;
unsigned long millis() { return fakeTime; }
void drawStatusBar() {
    ++statusReads;
    fakeTime+=drawDuration;
    // Battery text clearance at 320 px overlaps the HAS_TOUCH close glyph.
    if(overwriteStatusChrome)tft.paint(246,6,68,19,bruceConfig.bgColor);
}
void drawMainBorder() { fakeTime+=123; drawStatusBar(); }
int getColorVariation(int c) { return c; }
void TouchFooter() { ++footerDraws; }
"""
body = r"""
void pollStatus(uint8_t menuType, unsigned long& statusBarDrawTs) {
__TIMER__
}
unsigned long initStatus(bool compact,uint8_t menuType) {
__INIT__
    return statusBarDrawTs;
}
long area(const FakeTft& f) { long n=0; for(auto r:f.rects)n+=long(r.w)*r.h; return n; }
void scenario(int width,int height,int count,bool longLabels=true) {
    tftWidth=width;tftHeight=height;
    std::vector<Option> opts;
    for(int i=0;i<count;++i)opts.push_back({longLabels && i%3==0?"Large label requires the smaller font size":std::string(longLabels?"Item ":"I")+std::to_string(i),i%4!=1});
    FakeTft opt,ref;
    bool first=true;int previous=-1;
    const std::vector<int> moves={0,1,count-1,0,count/2,1,0};
    for(int index:moves) {
        index%=count;
        tft=ref;tft.resetCalls();drawSubmenuReference(index,opts,"Options");ref=tft;
        tft=opt;tft.resetCalls();const int reads=statusReads;
        drawSubmenu(index,opts,"Options",first,previous);opt=tft;
        assert(statusReads==reads);
        if(opt.pixels!=ref.pixels) {
            std::cerr << "Mismatch: "<<width<<"x"<<height<<" count="<<count<<" index="<<index<<" prev="<<previous<<" first="<<first<<"\n";
            for(size_t n=0;n<opt.pixels.size();++n)if(opt.pixels[n]!=ref.pixels[n]) {std::cerr<<"pixel "<<n%width<<","<<n/width<<" opt="<<opt.pixels[n]<<" ref="<<ref.pixels[n]<<"\n";break;}
        }
        assert(opt.pixels==ref.pixels);
        assert(opt.size==ref.size);
        if(!first) {
            assert(std::none_of(opt.texts.begin(),opt.texts.end(),[](auto text){return text.text=="Options";}));
            for(auto r:opt.rects) {
                // Only row-sized interior clears and old/new thumb rectangles on navigation.
                if(r.x==BORDER_OFFSET_FROM_SCREEN_EDGE+1)assert(r.h==std::max(FG,FM)*LH+std::max(FP*(LH/2),4));
                if(r.x==tftWidth-BORDER_OFFSET_FROM_SCREEN_EDGE)assert(r.h==tftHeight/count || r.h==tftHeight);
            }
            if(count>1 && index!=previous) assert(area(opt)<area(ref));
            if(!longLabels) {
                int thumbClears=0;
                for(auto r:opt.rects)if(r.x==tftWidth-BORDER_OFFSET_FROM_SCREEN_EDGE) {++thumbClears;assert(r.h==tftHeight/count);}
                assert(thumbClears==(previous!=index?2:0));
            }
        }
        bool selectedFound=false;
        for(auto text:opt.texts)if(text.text==opts[index].label && text.color==(opts[index].enabled?101:55))selectedFound=true;
        assert(selectedFound);
        previous=index;first=false;
    }
    // Full invalidation restores a screen repainted by a hover renderer and a changed title.
    tft=opt;tft.paint(6,STATUS_BAR_HEIGHT,tftWidth-12,tftHeight-STATUS_BAR_HEIGHT-6,777);
    drawSubmenu(0,opts,"Changed",true,previous);opt=tft;
    tft=ref;drawSubmenuReference(0,opts,"Changed");ref=tft;
    assert(opt.pixels==ref.pixels);
}
int main() {
    for(auto wh:std::vector<std::pair<int,int>>{{320,170},{170,320},{240,135},{320,240},{240,320},{480,320},{502,410},{1280,720}})
        for(int count: {1,2,3,4,7,20}) {scenario(wh.first,wh.second,count);scenario(wh.first,wh.second,count,false);}
    tftWidth=320;tftHeight=170;tft=FakeTft();
    std::vector<Option> shortOptions(7,{"Item",true});
    drawSubmenu(0,shortOptions,"Title",true,-1);tft.resetCalls();
    drawSubmenu(1,shortOptions,"Title",false,0);
    std::cout<<"Short-label navigation: "<<tft.rects.size()<<" fills, "<<area(tft)<<" cleared pixels, "<<tft.texts.size()<<" row text draws\n";
    tft=FakeTft();drawSubmenuReference(1,shortOptions,"Title");
    std::cout<<"Full-render reference: "<<tft.rects.size()<<" fills, "<<area(tft)<<" cleared pixels\n";
    tft=FakeTft();
    std::vector<Option> empty;
    drawSubmenu(0,empty,"Empty",true,-1);assert(tft.rects.empty());
    std::vector<Option> many(500,{"Item",true});
    drawSubmenu(499,many,"Many",true,-1);
    auto thumb=tft.rects.back();assert(thumb.y>=0 && thumb.y+thumb.h<=tftHeight && thumb.h==1);
    drawSubmenu(-1,many,"Many",false,499);thumb=tft.rects.back();assert(thumb.y==0 && thumb.h==1);
    fakeTime=100;drawDuration=25;statusReads=0;
    auto last=initStatus(false,MENU_TYPE_SUBMENU);assert(last==248 && statusReads==1);
    fakeTime=last+29999;pollStatus(MENU_TYPE_SUBMENU,last);assert(statusReads==1);
    fakeTime=last+30000;pollStatus(MENU_TYPE_SUBMENU,last);assert(statusReads==2 && last==fakeTime);
    fakeTime=last+30000;pollStatus(MENU_TYPE_MAIN,last);assert(statusReads==3);
    fakeTime=last+30000;pollStatus(MENU_TYPE_REGULAR,last);assert(statusReads==3);
    last=ULONG_MAX-10000;fakeTime=19998;pollStatus(MENU_TYPE_SUBMENU,last);assert(statusReads==3);
    fakeTime=19999;pollStatus(MENU_TYPE_SUBMENU,last);assert(statusReads==4 && last==fakeTime);
    fakeTime=900;last=initStatus(true,MENU_TYPE_SUBMENU);assert(statusReads==5 && last==925);
    submenuNeedsFullRedraw=false;redraw=false;
    fakeTime=last+30000;pollStatus(MENU_TYPE_SUBMENU,last);
#if defined(HAS_TOUCH)
    assert(submenuNeedsFullRedraw && redraw);
    tft=FakeTft();drawSubmenu(1,shortOptions,"Title",true,-1);
    auto beforeStatus=tft.pixels;
    submenuNeedsFullRedraw=false;redraw=false;overwriteStatusChrome=true;
    const int readsBeforeStatus=statusReads;
    fakeTime=last+30000;pollStatus(MENU_TYPE_SUBMENU,last);
    assert(submenuNeedsFullRedraw && redraw && tft.pixels!=beforeStatus);
    drawSubmenu(1,shortOptions,"Title",submenuNeedsFullRedraw,1);
    assert(tft.pixels==beforeStatus && statusReads==readsBeforeStatus+1);
#else
    assert(!submenuNeedsFullRedraw && !redraw); // Non-touch keeps its incremental fast path.
#endif
    std::cout << "PASS: framebuffer equivalence, navigation clear area, title invalidation, disabled/long labels, wrap, empty/huge lists, initial/idle/main/rollover status timer\n";
}
""".replace("__TIMER__",timer).replace("__INIT__",init)
compiler = shutil.which("g++")
if not compiler:
    raise SystemExit("BLOCKED: g++ is required for the submenu host regression")
with tempfile.TemporaryDirectory(prefix="bruce-submenu-host-") as tmp:
    tmp=Path(tmp)
    cpp=tmp/"submenu.cpp"
    cpp.write_text(preamble+renderer+reference+body,encoding="utf-8")
    env=dict(os.environ)
    env["PATH"]=str(Path(compiler).parent)+os.pathsep+env.get("PATH","")
    for flags in ([],["-DFM=1","-DFG=2"],["-DFG=4"],["-DFG=5"],["-DFP=2","-DFM=3","-DFG=6"],["-DFP=2","-DFM=4","-DFG=6"],["-DHAS_TOUCH"]):
        exe=tmp/"submenu.exe"
        subprocess.run([compiler,"-std=c++17","-Wall","-Wextra","-Werror","-Wno-sign-compare",*flags,str(cpp),"-o",str(exe)],check=True,env=env)
        print("Config:"," ".join(flags) or "default",flush=True)
        subprocess.run([str(exe)],check=True,env=env)
