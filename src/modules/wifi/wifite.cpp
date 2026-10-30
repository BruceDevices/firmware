// SPDX-License-Identifier: AGPL-3.0-or-later
#include <Arduino.h>
// CONFIG_IDF_TARGET_ESP32S3 comes from sdkconfig.h via Arduino.h; the vendored
// wifite core only builds for ESP32-S3 (enforced likewise in BruceWifite's
// build_private.py).
#if defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(LITE_VERSION)
#include "wifite.h"
#include <bruce_wifite.h>
#include <bruce_wifite_platform.h>
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/scrollableTextArea.h"
#include "core/display.h"
#include <memory>
#include <algorithm>
namespace {
void clearNavigation() {
    NextPress=PrevPress=UpPress=DownPress=SelPress=EscPress=false;
    AnyKeyPress=SerialCmdPress=false; forceMenuOption=-1;
}
int choose(const String &title,const std::vector<String> &labels,int initial=0) {
    int selected=-1; std::vector<Option> items;
    for(size_t i=0;i<labels.size();++i) items.emplace_back(labels[i],[&,i]{selected=i;});
    clearNavigation(); returnToMenu=false;
    loopOptions(items,MENU_TYPE_REGULAR,title.c_str(),initial,false,false,0,true);
    clearNavigation(); returnToMenu=false; return selected;
}
void view(const String &title,const String &text) { ScrollableTextArea area(title);area.fromString(text);area.show();clearNavigation(); }
String mac(const uint8_t *m) { char b[18];snprintf(b,sizeof(b),"%02X:%02X:%02X:%02X:%02X:%02X",m[0],m[1],m[2],m[3],m[4],m[5]);return b; }
String lock(const WfTarget &t) { return t.locked<0?"?":(t.locked?"LOCK":"CLEAR"); }
class App {
    std::vector<WfTarget> targets;
    std::vector<String> logs;
    WifiteJob *job=nullptr;
    uint32_t started=0;
    int selected=0;
    bool platform=false;
    void log(const String &s) {
        logs.push_back(String((millis()-started)/1000)+"s "+s);
        if(logs.size()>256) logs.erase(logs.begin());
    }
    bool drain() {
        WifiteEvent e; bool changed=false;
        while(job && wifite_poll(job,&e)) {
            changed=true;
            if(e.type==WifiteTarget) {
                auto it=std::find_if(targets.begin(),targets.end(),[&](const WfTarget &t){return !memcmp(t.bssid,e.target.bssid,6);});
                if(it!=targets.end()) *it=e.target;
                else if(targets.size()<WF_TARGET_MAX) targets.push_back(e.target);
            } else log(e.text);
        }
        return changed;
    }
    String logText() { String all;all.reserve(logs.size()*90);for(auto &s:logs) all+=s+"\n";return all; }
    bool start(WifiteMode mode,const WfTarget *target=nullptr,const char *pin=nullptr) {
        if(!platform) { platform=wf_platform_prepare(); if(!platform) {displayError("Wi-Fi initialization failed",true);return false;} }
        logs.clear();started=millis();if(mode==WifiteScan) {targets.clear();selected=0;}
        job=wifite_start(mode,target,pin);
        if(!job) {displayError("Cannot allocate WiFite worker",true);return false;}
        return true;
    }
    void finish() { drain();if(job) {wifite_free(job);job=nullptr;} }
    void background() {
        drawMainBorder();
        printTitle("WiFite");
    }
    void terminal() {
        ScrollableTextArea area("WiFite terminal");
        const auto bottom = [&area]() -> size_t {
            const size_t count=area.getMaxLines(), visible=area.getMaxVisibleLines();
            // The viewer reserves a row for its leading "..." when scrolled.
            // At an exact page boundary it also shows a trailing indicator.
            return visible>1 && count>=visible ? count-visible+1 : 0;
        };
        bool dirty=true,follow=true,stopping=false;
        uint32_t lastDraw=0;
        clearNavigation();
        for(;;) {
            if(drain()) dirty=true;
            if(job && wifite_finished(job)) {finish();dirty=true;}
            if(check(EscPress)) {
                if(!job) return;
                wifite_cancel(job);stopping=true;log("Stopping; waiting for radio cleanup");dirty=true;
            }
            if(check(SelPress)) {follow=true;dirty=true;}
            bool up=check(PrevPress);if(check(UpPress)) up=true;
            bool down=check(NextPress);if(check(DownPress)) down=true;
            if(up) {follow=false;area.scrollUp();}
            if(down) {
                if(area.firstVisibleLine<bottom()) ++area.firstVisibleLine;
                follow=area.firstVisibleLine>=bottom();
            }
            if(dirty && millis()-lastDraw>=100) {
                size_t position=area.firstVisibleLine;
                area.fromString(logText());
                area.firstVisibleLine=follow?bottom():std::min(position,bottom());
                area.draw(true);dirty=false;lastDraw=millis();
            } else if(up||down) area.draw(true);
            if(stopping && !job) return;
            delay(10);
        }
    }
    void details(const WfTarget &t) {
        view("Target details",String(t.ssid)+"\nBSSID: "+mac(t.bssid)+"\nChannel: "+String(t.channel)+
            "  RSSI: "+String(t.rssi)+" dBm\nWPS: "+(t.wps?"advertised":"not observed")+
            "\nPIN lock: "+lock(t)+"\nObserved clients: "+String(t.clients)+
            "\nWPS version: "+String(t.version,HEX)+"\nMethods: 0x"+String(t.methods,HEX)+
            "\nManufacturer: "+t.manufacturer+"\nModel: "+t.model);
    }
    void actions(size_t index) {
        const WfTarget t=targets[index];
        for(;;) {
            background();
            int a=choose(t.ssid,{"Auto test","Pixie Dust","WPS brute force","Test one PIN","Target details","Back"});
            if(a<0||a==5) return;
            if(a==4) {details(t);continue;}
            if(!t.wps) {displayError("WPS not advertised by this AP",true);continue;}
            String pin="12345670";
            if(a==3) {pin=num_keyboard(pin,8,"WPS PIN (8 digits)");
                if(pin.length()!=8 || !std::all_of(pin.begin(),pin.end(),[](char c){return c>='0'&&c<='9';})) {displayError("Enter exactly 8 digits",true);continue;} }
            const WifiteMode modes[]={WifiteAuto,WifitePixie,WifiteBruteforce,WifiteVerifyPin};
            if(start(modes[a],&t,pin.c_str())) terminal();
        }
    }
    void table(bool scanning) {
        // Keep the scan overview independent of the RF feature and repaint only
        // changed rows, so incoming beacons do not flash the whole display.
        std::vector<String> paintedRows;
        String paintedInfo,paintedStatus;
        int scroll=0,paintedSelected=-1,top=0;
        bool force=true,stopping=false,open=false;uint32_t lastDraw=0;bool dirty=true;
        clearNavigation();
        for(;;) {
            if(drain()) dirty=true;
            if(job && wifite_finished(job)) {finish();scanning=false;dirty=true;}
            if(stopping && !job) { if(open && !targets.empty()) {actions(selected);force=true;dirty=true;stopping=false;open=false;} else return; }
            if(check(EscPress)) {if(job) {wifite_cancel(job);stopping=true;dirty=true;} else return;}
            if(check(SelPress) && !targets.empty()) {
                if(job) {wifite_cancel(job);stopping=true;open=true;dirty=true;}
                else {actions(selected);force=true;dirty=true;}
            }
            bool up=check(PrevPress);if(check(UpPress)) up=true;
            bool down=check(NextPress);if(check(DownPress)) down=true;
            if(!targets.empty()&&(up||down)) {selected=(selected+(up?-1:1)+targets.size())%targets.size();dirty=true;}
            if(dirty && millis()-lastDraw>=100) {
                if(force) {
                    drawMainBorder(); printTitle("WiFite networks");
                    top=tft.getCursorY()+3;
                    paintedRows.clear();paintedInfo="";paintedStatus="";paintedSelected=-1;
                }
                tft.setTextFont(1);tft.setTextSize(1);tft.setTextDatum(TL_DATUM);
                tft.setTextWrap(false);
                const int left=6,width=tftWidth-12,rowHeight=14;
                const int columns=width/6,ssidColumns=std::max(4,columns-23);
                const int footer=tftHeight-36;
                const int visible=std::max(1,(footer-top-14)/rowHeight);
                if(selected<scroll) scroll=selected;
                if(selected>=scroll+visible) scroll=selected-visible+1;
                auto fit=[](String text,int chars) {
                    // SSIDs may contain control bytes; never let them break rows.
                    for(unsigned i=0;i<text.length();++i)
                        if((uint8_t)text[i]<32 || (uint8_t)text[i]>126) text.setCharAt(i,'?');
                    if((int)text.length()>chars) text=text.substring(0,chars-1)+"~";
                    while((int)text.length()<chars) text+=' ';
                    return text;
                };
                auto rowText=[&](const WfTarget &t) {
                    char values[40];
                    snprintf(values,sizeof(values)," %-3s %-4s %3u %2u %4d",
                        t.wps?"YES":"--",t.locked<0?"?":t.locked?"LOCK":"OPEN",
                        (unsigned)t.clients,(unsigned)t.channel,(int)t.rssi);
                    return fit(t.ssid[0]?String(t.ssid):String("<hidden>"),ssidColumns)+values;
                };
                auto paint=[&](const String &text,int y,bool highlight) {
                    uint16_t bg=highlight?bruceConfig.priColor:bruceConfig.bgColor;
                    uint16_t fg=highlight?bruceConfig.bgColor:bruceConfig.priColor;
                    tft.fillRect(left,y,width,rowHeight,bg);
                    tft.setTextColor(fg,bg);tft.drawString(text,left,y+2,1);
                };
                if(force) {
                    paint(fit("SSID",ssidColumns)+" WPS LOCK CLI CH  dBm",top,false);
                    tft.drawFastHLine(left,top+12,width,bruceConfig.priColor);
                }
                paintedRows.resize(visible);
                for(int row=0;row<visible;++row) {
                    int index=scroll+row;
                    String text=index<(int)targets.size()?rowText(targets[index]):String("");
                    if(targets.empty() && row==0) text=scanning?"Waiting for networks...":"No networks. Start a scan.";
                    bool highlighted=!targets.empty() && index==selected;
                    if(force || text!=paintedRows[row] ||
                       highlighted!=(row==paintedSelected)) {
                        paint(text,top+14+row*rowHeight,highlighted);paintedRows[row]=text;
                    }
                }
                paintedSelected=targets.empty()?-1:selected-scroll;
                String info;
                if(!targets.empty()) {
                    const auto &t=targets[selected];
                    info=fit(t.ssid[0]?String(t.ssid):String("<hidden>"),columns)+"\n"+mac(t.bssid);
                }
                String status=String(targets.empty()?0:selected+1)+"/"+String(targets.size())+" "+
                    (stopping?"Stopping...":scanning?"Scan SEL:select BACK:stop":"SEL:actions BACK:menu");
                if(force || info!=paintedInfo || status!=paintedStatus) {
                    tft.fillRect(left,footer,width,34,bruceConfig.bgColor);
                    tft.setTextColor(bruceConfig.priColor,bruceConfig.bgColor);
                    if(!targets.empty()) {
                        tft.drawString(info.substring(0,info.indexOf('\n')),left,footer,1);
                        tft.drawString(mac(targets[selected].bssid),left,footer+10,1);
                    }
                    tft.drawString(status,left,footer+23,1);
                    paintedInfo=info;paintedStatus=status;
                }
                tft.setTextWrap(true);force=false;
                lastDraw=millis();dirty=false;
            }
            delay(10);
        }
    }
    void saveLog() {
        if(logs.empty()) {displayError("No log yet",true);return;}
        if(!setupSdCard()) {displayError("Insert an SD card",true);return;}
        FS *fs=&SD;
        File f=createNewFile(fs,"/apps_data/wifite","log.txt");
        if(!f) {displayError("Cannot create log",true);return;}
        String text=logText();bool ok=f.print(text)==text.length();f.flush();f.close();
        if(ok) displaySuccess("Log saved to SD",true);else displayError("Incomplete log write",true);
    }
public:
    ~App() {
        if(job) {wifite_cancel(job);while(!wifite_finished(job)) {drain();delay(10);}finish();}
        if(platform) wf_platform_restore();
    }
    void run() {
        setupSdCard();
        for(;;) {
            background();
            int a=choose("WiFite",{"Scan networks","Router table","Last terminal log","Save log to SD","About / limits","Back"});
            if(a<0||a==5) return;
            if(a==0 && start(WifiteScan)) table(true);
            if(a==1) table(false);
            if(a==2) view("Last terminal log",logText());
            if(a==3) saveLog();
            if(a==4) view("WiFite","Native port of the WirelessPi WiFite app. No Pi or JS required.\n"
                "Auto, Pixie Dust and WPS brute force test 12345670 first.\n"
                "Auto then runs Pixie and resumable PIN search. One PIN tests only your supplied PIN.\n"
                "PIN lock is honored; waits passively up to 30 seconds. BACK cancels.\n"
                "CLI counts observed clients, not a complete AP client list.\n"
                "Experimental WPS transport; hardware compatibility varies. This is not the full Linux Wifite CLI.\n"
                "Use with networks you own or have permission to test.");
        }
    }
};
}
void wifite_menu() {std::unique_ptr<App> app(new App);app->run();}
#endif
