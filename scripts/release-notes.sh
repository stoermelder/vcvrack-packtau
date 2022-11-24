#!/bin/bash

cat <<- EOH
## Automatically generated PackTau build

This build is automatically generated every time I push my changes. As such it is the latest version of the code and may be unstable, unusable, unsuitable for human consumption, and so on.

These assets were built against
https://vcvrack.com/downloads/Rack-SDK-2.1.0-win.zip
https://vcvrack.com/downloads/Rack-SDK-2.1.0-mac.zip
https://vcvrack.com/downloads/Rack-SDK-2.1.0-lin.zip

Used build environments:
macos-11
windows-2019
ubuntu-20.04

The build date and most recent commits are:
EOH
date
echo ""
echo "Most recent commits:"
echo ""
git log --pretty=oneline | head -5
