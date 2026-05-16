#!/bin/bash

rm -rf libtinysoundfont
mkdir libtinysoundfont
cp TinySoundFont/tsf.h TinySoundFont/README* TinySoundFont/LICENSE TinySoundFont/fast*.h TinySoundFont/cast.h libtinysoundfont/.
cd TinySoundFont/examples
bash build-linux-gcc.sh
cd ../..
./TinySoundFont/examples/dump-linux-x86_64 midi-sources/1mgm.sf2 libtinysoundfont/1mgm.h
./TinySoundFont/examples/dump-linux-x86_64 midi-sources/Scratch2010.sf2 libtinysoundfont/scratch2010.h
for i in venture furelise jm_mozdi; do
    xxd -i midi-sources/$i.mid | sed '$ d' | sed 's/unsigned.*/const unsigned char _mid[] PROGMEM = {/' > ../examples/PlayMIDIFromROM/"$i"_mid.h
done
rm -r ../src/libtinysoundfont
mv libtinysoundfont ../src/.
