/* SPDX-License-Identifier: GPL-3.0-or-later
 * ProtoPirate 4.0 Bruce interpreter frontend, RX1 / native UI.
 * RocketGod / The Pirates' Plunder protocol core: 4c9afe58f881a28d.
 * Requires the matching Bruce protoInfo/protoDecode/protoEncode API v1.
 * Documentation: https://github.com/BruceDevices/Wiki/blob/main/docs/features/protopirate.md
 */
var PP = (function () {
    function field(text, name) {
        var lines = text.split('\n');
        for (var i=0; i<lines.length; i++) {
            var p=lines[i].indexOf(':');
            if(p>=0 && lines[i].substring(0,p)===name) return lines[i].substring(p+1).replace(/^\s+|\s+$/g,'');
        }
        return null;
    }
    function setField(text, name, value) {
        var lines=text.split('\n'),out='',found=false;
        for(var i=0;i<lines.length;i++) {
            if(lines[i].substring(0,name.length+1)===name+':') {
                if(!found)out+=name+': '+value+'\n';found=true;
            } else if(lines[i].length)out+=lines[i]+'\n';
        }
        return found?out:out+name+': '+value+'\n';
    }
    function numeric(text,name) {
        var s=field(text,name);
        if(s===null || !/^\d+$/.test(s))return null;
        var n=Number(s);return isFinite(n)&&n>=0&&n<=4294967295?n:null;
    }
    function rawPulses(text) {
        if(typeof text!=='string'||text.length>262144)throw new Error('Capture is too large');
        var lines=text.split('\n'),out=[];
        for(var i=0;i<lines.length;i++) {
            if(lines[i].substring(0,9)!=='RAW_Data:'&&lines[i].substring(0,9)!=='Data_RAW:')continue;
            var tokens=lines[i].substring(9).split(/\s+/);
            for(var j=0;j<tokens.length;j++)if(tokens[j]) {
                if(!/^-?\d+$/.test(tokens[j]))throw new Error('Malformed RAW duration');
                var n=Number(tokens[j]);if(!n||Math.abs(n)>1000000)throw new Error('Invalid RAW duration');
                if(out.length>=16384)throw new Error('Too many RAW durations');out.push(n);
            }
        }
        if(!out.length)throw new Error('No RAW durations');return out;
    }
    function timings(text) {
        var p=rawPulses(text),sum=0,min=1000001,max=0,high=0,low=0,bins=[];
        for(var i=0;i<p.length;i++) {
            var n=Math.abs(p[i]);sum+=n;if(n<min)min=n;if(n>max)max=n;
            if(p[i]>0)high++;else low++;
            var bucket=Math.round(n/50)*50,found=false;
            for(var j=0;j<bins.length;j++)if(bins[j].us===bucket){bins[j].count++;found=true;break;}
            if(!found&&bins.length<128)bins.push({us:bucket,count:1});
        }
        bins.sort(function(a,b){return b.count-a.count;});
        return {count:p.length,min:min,max:max,mean:Math.round(sum/p.length),duration:sum,high:high,low:low,bins:bins};
    }
    function api(r) {
        if(typeof r==='string')r=JSON.parse(r);
        if(!r||!r.ok)throw new Error(r&&r.error?r.error:'Protocol engine failed');return r;
    }
    function receiveProfile(content) {
        var hz=numeric(content,'Frequency'),preset=field(content,'Bruce_RX_Preset')||field(content,'Preset');
        if(hz===null||!((hz>=300000000&&hz<=348000000)||(hz>=387000000&&hz<=464000000)||(hz>=779000000&&hz<=928000000)))throw new Error('Missing or invalid receive frequency');
        if(preset)preset=preset.replace(/^FuriHalSubGhzPreset/,'');
        if(preset!=='Ook650Async'&&preset!=='Ook270Async'&&preset!=='2FSKDev476Async'&&preset!=='2FSKDev238Async')throw new Error('This file needs a receive preset that is not supported by this app');
        return {frequency:hz/1000000,preset:preset};
    }
    function nextCounter(protocol, counter, nativeCounter) {
        if(counter===null)return null;
        // Match the upstream emulation scene: advance once per explicit send.
        var next=(counter+1)>>>0;
        if(nativeCounter)return next&65535;
        if(protocol==='Kia V1'||protocol==='Kia V2')return next&4095;
        if(protocol==='Ford V0')return next&524287;
        if(protocol==='Renault V0')return next&255;
        return next;
    }
    function emulationData(data, button, counter) {
        var result=data;
        if(button!==null){if(button<0||button>255||Math.floor(button)!==button)throw new Error('Invalid button');result=setField(result,field(data,'Btn')!==null?'Btn':'Button',button);}
        if(counter!==null){if(counter<0||counter>4294967295||Math.floor(counter)!==counter)throw new Error('Invalid counter');result=setField(result,field(data,'Cnt')!==null?'Cnt':'Counter',counter);}
        return setField(result,'Repeat',3);
    }
    return {field:field,setField:setField,numeric:numeric,rawPulses:rawPulses,timings:timings,api:api,receiveProfile:receiveProfile,emulationData:emulationData,nextCounter:nextCounter};
}());

function runProtoPirate() {
    var ui=require('menu'),dialog=require('dialog'),k=require('keyboard'),rf=require('subghz'),fs=require('storage');
    var history=[],historyBytes=0;
    var presets=['Ook650Async','2FSKDev476Async','Ook270Async','2FSKDev238Async'];
    var labels=['AM650','FM476','AM270','FM238'];
    var frequencies=[315,433.92,433.65,434.25,434.42,868.35];
    var presetIndex=-1,frequencyIndex=1,hopFrequency=false,cycle=0,saveId=0,receiveRequested=false;
    var info;
    function release(){delay(170);}
    function menu(label,items,selected){
        if(!items.length)return -1;
        // Native menus start on their first item. Rotate the requested default
        // into that slot, preserving Cancel as the default for every TX prompt.
        var start=selected||0,choices=[];
        if(start<0||start>=items.length)start=0;
        for(var i=0;i<items.length;i++)choices.push(items[(start+i)%items.length]);
        var choice=ui.show(label,choices);
        return choice<0||choice>=items.length?-1:(choice+start)%items.length;
    }
    function view(label,message){
        ui.showMainBorder();
        dialog.viewText(String(message),label);
        release();
    }
    function status(label,message){
        ui.showMainBorder();
        var viewer=dialog.createTextViewer(label+'\n\n'+message);
        try{viewer.draw();}finally{viewer.close();}
    }
    function busy(s){dialog.info(s,false);}
    function safe(fn){try{return fn();}catch(e){dialog.error(String(e),true);return null;}}
    function sd(path){return {fs:'sd',path:path};}
    function save(content,label){
        var path='/subghz/ProtoPirate/PP_'+Date.now()+'_'+(++saveId)+'_'+label+'.sub';
        if(!fs.write(sd(path),content,'write'))throw new Error('SD write failed');dialog.success('Saved',false);view('Saved file',path);return path;
    }
    function addHistory(item){
        var size=item.raw.length+item.data.length;
        while(history.length&&(history.length>=12||historyBytes+size>200000)){historyBytes-=history[0].raw.length+history[0].data.length;history.shift();}
        if(size<=200000){history.push(item);historyBytes+=size;}
    }
    function send(raw){
        PP.rawPulses(raw);
        if(typeof rf.protoTransmit!=='function')throw new Error('Matching ProtoPirate firmware TX API is missing');
        busy('Transmitting...');
        if(!rf.protoTransmit(raw))throw new Error('Radio or RMT transmission failed');
    }
    function replay(item){
        if(!item.raw)throw new Error('This decoded file has no original RAW capture');
        if(PP.field(item.raw,'Bruce_Buffer_Full')==='1')throw new Error('Capture is truncated; record a complete frame first');
        if(menu('Replay original RAW?',['Transmit once','Cancel'],1)===0){send(item.raw);view('Transmission complete','Radio waveform sent. Receiver acceptance cannot be verified by the transmitter.');}
    }
    function emulate(item){
        if(!item.encoder)throw new Error(item.validation==='failed'?'Decoder reported a checksum failure. Capture a complete valid frame before emulating.':'No upstream encoder for this protocol');
        var nativeCounter=PP.field(item.data,'Cnt')===null,counterField=nativeCounter?'Counter':'Cnt',buttonField=PP.field(item.data,'Btn')===null?'Button':'Btn';
        var counter=item.fieldEditing===false?null:PP.numeric(item.data,counterField),button=item.fieldEditing===false?null:PP.numeric(item.data,buttonField);
        for(;;){var items=['Encode + transmit','Next counter: '+(counter===null?'unavailable':PP.nextCounter(item.name,counter,nativeCounter)),'Button: '+(button===null?'unavailable':button),'Preview encoded waveform','Back'];
            var choice=menu(item.name+' / Emulate',items,4);
            if(choice<0||choice===4)return;
            if(choice===1){if(counter!==null)counter=PP.nextCounter(item.name,counter,nativeCounter);continue;}
            if(choice===2){if(button!==null)button=(button+1)&(item.name==='Kia V1'?255:15);continue;}
            var source=PP.emulationData(item.data,button,PP.nextCounter(item.name,counter,nativeCounter)),encoded=PP.api(rf.protoEncode(source));
            if(choice===3){view('Encoder preview',encoded.pulses+' durations\n'+encoded.updated);continue;}
            if(menu('Send encoded signal?',['Transmit','Cancel'],1)!==0)continue;
            send(encoded.raw);
            item.data=encoded.updated;
            var refreshed=PP.api(rf.protoDecode(item.data));
            if(refreshed.frames.length)item.text=refreshed.frames[0].text;
            counter=item.fieldEditing===false?null:PP.numeric(item.data,counterField);button=item.fieldEditing===false?null:PP.numeric(item.data,buttonField);
            view('Transmission complete','Waveform sent. Saved in this session. Use Save decoded to keep the updated counter.');
        }
    }
    function showTimings(raw){
        var t=PP.timings(raw),s='Durations: '+t.count+'\nHIGH / LOW: '+t.high+' / '+t.low+'\nTotal: '+Math.round(t.duration/1000)+' ms\nMin / max: '+t.min+' / '+t.max+' us\nMean: '+t.mean+' us\n\nTiming groups (50 us bins):\n';
        for(var i=0;i<Math.min(10,t.bins.length);i++)s+=t.bins[i].us+' us: '+t.bins[i].count+'\n';view('Timing analyzer',s);
    }
    function matchReceiver(item){
        var profile=PP.receiveProfile(item.raw||item.data),fi=-1;
        for(var i=0;i<frequencies.length;i++)if(frequencies[i]===profile.frequency)fi=i;
        // Keep at most one additional frequency from an imported file.
        if(fi<0){if(frequencies.length>6)frequencies.pop();fi=frequencies.length;frequencies.push(profile.frequency);}
        frequencyIndex=fi;
        for(i=0;i<presets.length;i++)if(presets[i]===profile.preset)presetIndex=i;
        hopFrequency=false;cycle=0;receiveRequested=true;
    }
    function result(item){
        for(;;){var choice=menu(item.name,['View decoded fields','Replay original RAW','Emulate / encode','Save original RAW','Save decoded .sub','Timing analyzer','Receive matching signal','Continue / back'],0);
            if(choice<0||choice===7)return;
            safe(function(){
                if(choice===0)view(item.name,item.text+'\n\n'+item.data);
                else if(choice===1)replay(item);
                else if(choice===2)emulate(item);
                else if(choice===3){if(!item.raw)throw new Error('No RAW capture');save(item.raw,'RAW');}
                else if(choice===4){if(!item.data)throw new Error('No decoded record');save(item.data,'decoded');}
                else if(choice===5)showTimings(item.raw);
                else if(choice===6)matchReceiver(item);
            });
            if(receiveRequested)return;
        }
    }
    function analyze(content,live){
        if(!live)busy('Decoding captured data...');var decoded=PP.api(rf.protoDecode(content)),raw=PP.field(content,'Protocol')==='RAW'?content:'';
        if(!decoded.frames.length){var unknown={name:'Unrecognized signal',text:PP.field(content,'Bruce_Buffer_Full')==='1'?'The capture buffer filled. This waveform may be incomplete; capture it again.':'No complete supported frame was decoded. Check the transmitter frequency and preset, then capture again. A decoded .sub file can load without verifying its over-air waveform.',data:'',raw:raw,encoder:false};if(live)return unknown;addHistory(unknown);result(unknown);return null;}
        var entries=[],seen={};
        for(var i=0;i<decoded.frames.length;i++){
            var f=decoded.frames[i],key=f.protocol+'\n'+f.data;if(seen[key])continue;seen[key]=true;
            var item={name:f.protocol,text:f.text,data:f.data,raw:raw,encoder:f.encoder,validation:f.validation,fieldEditing:f.fieldEditing};entries.push(item);addHistory(item);
        }
        var chosen=0;if(entries.length>1){var options=[];for(i=0;i<entries.length;i++)options.push(entries[i].name+' #'+(i+1));chosen=menu('Decoded frames',options,0);}
        if(chosen>=0)result(entries[chosen]);
        return null;
    }
    function receive(){
        var lastUnknown=null,misses=0,slot=-1,slotStarted=0;
        for(;;){
            if(k.getEscPress()){if(lastUnknown)addHistory(lastUnknown);release();return;}
            var scanning=presetIndex<0||hopFrequency;
            // A complete Chrysler three-repeat burst is about 2.69 seconds.
            // The native RMT read completes after the burst, not after each bit.
            // Hold auto slots for 12 seconds, and locked profiles for 30 seconds.
            // Noise must not advance the scanner or open a blocking result menu.
            if(slot!==cycle){slot=cycle;slotStarted=Date.now();}
            var pi=presetIndex<0?cycle%presets.length:presetIndex;
            var fi=hopFrequency?Math.floor(cycle/(presetIndex<0?presets.length:1))%frequencies.length:frequencyIndex;
            var timeout=scanning?Math.max(1,Math.ceil((12000-(Date.now()-slotStarted))/1000)):30;
            rf.setFrequency(frequencies[fi]);
            status('Receiving / '+labels[pi],frequencies[fi]+' MHz\nPress your remote repeatedly\n'+
                (scanning?'Auto: 12 seconds per profile':'Locked to this frequency / preset')+
                (misses?'\nUnmatched captures: '+misses:'')+'\nBack: stop / keep last unmatched RAW');
            var raw=rf.readRaw(timeout,presets[pi]);
            if(k.getEscPress()){if(lastUnknown)addHistory(lastUnknown);release();return;}
            if(raw&&raw.length){
                if(PP.field(raw,'Bruce_RX_Preset')!==presets[pi])throw new Error('Firmware did not confirm the requested receive preset');
                var unknown=analyze(raw,true);
                if(unknown){lastUnknown=unknown;misses++;}
                else {slotStarted=Date.now();if(receiveRequested)return;}
            }
            if(scanning&&Date.now()-slotStarted>=12000)cycle++;
            delay(10);
        }
    }
    function browse(){
        var path='/';
        for(;;){var selected=dialog.pickFile(path,'SUB');if(!selected)return;
            path=selected.substring(0,selected.lastIndexOf('/'))||'/';
            var name=selected.substring(selected.lastIndexOf('/')+1),list=fs.readdir(path,{withFileTypes:true}),entry=null;
            for(var i=0;i<list.length;i++)if(list[i].name===name&&!list[i].isDirectory){entry=list[i];break;}
            if(!entry)throw new Error('Selected file is no longer available');
            if(entry.size>262144)throw new Error('File exceeds 256 KiB limit');
            analyze(fs.read(selected));if(receiveRequested)return;
        }
    }
    function settings(){for(;;){var c=menu('Receiver settings',['Frequency: '+frequencies[frequencyIndex]+' MHz','Preset: '+(presetIndex<0?'Auto':labels[presetIndex]),'Frequency hopping: '+(hopFrequency?'ON':'OFF'),'Back'],0);
        if(c<0||c===3)return;
        if(c===0){var opts=[];for(var i=0;i<frequencies.length;i++)opts.push(frequencies[i]+' MHz');var n=menu('Frequency',opts,frequencyIndex);if(n>=0)frequencyIndex=n;}
        if(c===1){n=menu('Preset',['Auto',labels[0],labels[1],labels[2],labels[3]],presetIndex+1);if(n>=0)presetIndex=n-1;}
        if(c===2)hopFrequency=!hopFrequency;
    }}
    try{
        if(typeof rf.protoInfo!=='function'||typeof rf.protoDecode!=='function'||typeof rf.protoEncode!=='function')throw new Error('Install the matching ProtoPirate 4.0 Bruce firmware first. This JS app requires API v1.');
        info=PP.api(rf.protoInfo());if(info.api!==1)throw new Error('Incompatible protocol API version');
        if(info.emulationEnabled!==true)throw new Error('This app requires emulation enabled in the firmware build');
        for(;;){var action=receiveRequested?0:menu('4.0 DEV RX1 / Emulation ON',['Receive signal','Sub Decode / files','Signal history','Receiver settings','Protocol catalogue','About / limitations','Exit'],0);
            receiveRequested=false;
            if(action<0||action===6)break;
            safe(function(){
                if(action===0)receive();
                else if(action===1)browse();
                else if(action===2){if(!history.length){view('History','No captures this session');return;}var names=[];for(var i=history.length-1;i>=0;i--)names.push(history[i].name);var index=menu('Signal history',names,0);if(index>=0)result(history[history.length-1-index]);}
                else if(action===3)settings();
                else if(action===4){var msg='Shared Bruce beta + ProtoPirate 4.0\nEmulation enabled by default\n';for(i=0;i<info.protocols.length;i++)msg+=info.protocols[i].name+': RX'+(info.protocols[i].encoder?' + encoder':'')+'\n';view('Protocol catalogue',msg+'\nEncoder availability does not imply decrypted fields or receiver acceptance.');}
                else view('About this port','RocketGod / The Pirates\' Plunder\nBruce JS frontend + original C protocol engine\nRX fix 1: longer capture / noise retry\n\nHost tests are not RF hardware tests.\n\nAM650/270 and FM476/238 live capture. F4/Honda-specific custom radio presets are not yet ported.\n\nDevice-encrypted keystores, brute-force recovery, model database and remote analyzer are not ported. Encrypted frames can remain undeciphered.\n\nAuto mode visits presets/frequencies sequentially; a single CC1101 cannot listen to all at once. Open a known .sub file and choose Receive matching signal to lock its frequency and preset.');
            });
        }
    }catch(e){dialog.error(String(e),true);}
}
if(typeof PP_TEST==='undefined'||!PP_TEST)runProtoPirate();
