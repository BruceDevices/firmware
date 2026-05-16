#!/usr/bin/env python3
import sys
import struct
import subprocess
import re
import os
import os.path
import argparse
import time
import shutil
import json

def main():
    parser = argparse.ArgumentParser(description='Version updater')
    parser.add_argument('-v', '--version', action='store', required=True, help='Version in X.Y.Z form')
    args = parser.parse_args()

    major, minor, sub = args.version.split(".")
    # Silly way to check for integer x.y.z
    major = int(major)
    minor = int(minor)
    sub = int(sub)

    # library.properties
    with open("library.properties", "r") as fin:
        with open("library.properties.new", "w") as fout:
            for l in fin:
                if l.startswith("version="):
                    l = "version=" + str(args.version) + "\n"
                fout.write(l);
    shutil.move("library.properties.new", "library.properties")

    # package.json
    with open("library.json", "r") as fin:
        library = json.load(fin);
        library["version"] = str(args.version);
        with open("library.json.new", "w") as fout:
            json.dump(library, fout, indent = 4);
    shutil.move("library.json.new", "library.json")

    # src/ESP8266AudioVer.h
    with open("src/ESP8266AudioVer.h", "w") as fout:
        fout.write("#pragma once\n")
        fout.write("#define ESP8266AUDIO_MAJOR " + str(major) + "\n")
        fout.write("#define ESP8266AUDIO_MINOR " + str(minor) + "\n")
        fout.write("#define ESP8266AUDIO_REVISION " + str(sub) + "\n")
        fout.write('#define ESP8266AUDIO_VERSION_STR "' + str(args.version) + '"' + "\n")

main()
