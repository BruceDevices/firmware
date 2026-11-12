import configparser
import subprocess
import sys
from os import listdir
from os.path import isdir, isfile, join
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    Import: Any = None
    env: Any = {}

Import("env")  # type: ignore

PROJECT_DIR = env.subst("$PROJECT_DIR")
GITMODULES = join(PROJECT_DIR, ".gitmodules")


def get_submodule_paths():
    if not isfile(GITMODULES):
        return []
    parser = configparser.ConfigParser()
    parser.read(GITMODULES, encoding="utf-8")
    return [parser[s]["path"] for s in parser.sections() if "path" in parser[s]]


def is_missing(path):
    full = join(PROJECT_DIR, path)
    # An uninitialized submodule is an empty directory (or doesn't exist at all)
    return not isdir(full) or not [f for f in listdir(full) if f != ".git"]


missing = [p for p in get_submodule_paths() if is_missing(p)]

if missing:
    print("Missing git submodules: " + ", ".join(missing))
    if not isdir(join(PROJECT_DIR, ".git")) and not isfile(join(PROJECT_DIR, ".git")):
        sys.stderr.write(
            "Error: project is not a git repository, can't fetch submodules.\n"
            "Clone the repo with 'git clone --recursive' or download the missing libraries manually.\n"
        )
        env.Exit(1)

    print("Running 'git submodule update --init --recursive'...")
    try:
        subprocess.check_call(["git", "submodule", "update", "--init", "--recursive"], cwd=PROJECT_DIR)
    except (OSError, subprocess.CalledProcessError) as e:
        sys.stderr.write("Error: failed to update git submodules: %s\n" % e)
        env.Exit(1)

    still_missing = [p for p in missing if is_missing(p)]
    if still_missing:
        sys.stderr.write("Error: submodules still missing: %s\n" % ", ".join(still_missing))
        env.Exit(1)
    print("Git submodules ready.")
