Publishing New Releases
=======================

First, update the version number throughout the repo and push the change:

    ./tools/makever.py --version X.Y.Z
    git commit -a -m "Update version"
    git push

Then tag it

    git tag X.Y.Z
    git push origin X.Y.Z

Then on the GH web interface it will generate a draft release from that tag
that can be released if proper through the GUI.
