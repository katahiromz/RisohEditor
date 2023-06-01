#!/bin/bash
# pack.sh --- RisohEditor deploy script
################################################################################
# RisohEditor --- Another free Win32 resource editor
# Copyright (C) 2017-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
################################################################################

# TODO: Update the version number
RE_VERSION=5.7.6
RE_NAME="RisohEditor-$RE_VERSION-no-inst"
RE_BIN_DIR="build/$RE_NAME"
RE_FILES="LICENSE.txt Standardize.md HYOJUNKA.txt TRANSLATORS.txt src/resource.h build/RisohEditor.exe mcdx/MESSAGETABLEDX.md"
RE_TARGET="build/RisohEditor-$RE_VERSION-no-inst.zip"

################################################################################

if ! which zip > /dev/null 2>&1; then
    echo ERROR: there is no executable zip.
    exit 10
fi

if [ ! -d build ]; then
    echo ERROR: the build directory doesn\'t exists.
    exit 1
fi

if [ ! -d data ]; then
    echo ERROR: the data directory doesn\'t exists.
    exit 2
fi

if [ ! -e build/RisohEditor.exe ]; then
    echo ERROR: build/RisohEditor.exe doesn\'t exists.
    exit 3
fi

if [ ! -e build/mcdx.exe ]; then
    echo ERROR: build/mcdx.exe doesn\'t exists.
    exit 4
fi

if [ -e "$RE_BIN_DIR" ]; then
    echo Deleting "$RE_BIN_DIR"...
    rm -fr "$RE_BIN_DIR"
    mkdir "$RE_BIN_DIR"
else
    mkdir "$RE_BIN_DIR"
fi

################################################################################

cp $RE_FILES "$RE_BIN_DIR"
cp -r data "$RE_BIN_DIR"
cp build/mcdx.exe "$RE_BIN_DIR/data/bin"

mkdir "$RE_BIN_DIR/OLE"
cp -f src/MOleHost.hpp src/MOleHost.cpp src/MWindowBase.hpp "$RE_BIN_DIR/OLE"

mkdir "$RE_BIN_DIR/MyWndCtrl"
cp -f "MyWndCtrl/MyWndCtrl.cpp" "MyWndCtrl/MWindowBase.hpp" "MyWndCtrl/CMakeLists.txt" "$RE_BIN_DIR/MyWndCtrl"
cp -f build/MyWndCtrl.dll "$RE_BIN_DIR/MyWndCtrl"

mkdir "$RE_BIN_DIR/DlgInit"
cp -f "src/DlgInit.h" "$RE_BIN_DIR/DlgInit"
mkdir "$RE_BIN_DIR/Toolbar"
cp -f "src/Toolbar.h" "$RE_BIN_DIR/Toolbar"

mkdir "$RE_BIN_DIR/EGA"
cp -f "EGA/EGA-Manual.pdf" "$RE_BIN_DIR/EGA"
cp -f EGA/samples/*.ega EGA-samples/*.ega "$RE_BIN_DIR/EGA"

cp -r README*.txt "$RE_BIN_DIR"
cp -r HISTORY*.txt "$RE_BIN_DIR"
cp -r win32-samples "$RE_BIN_DIR"

cd build
if zip -9 -r -q "$RE_NAME.zip" "$RE_NAME"; then
    cd ..
    if [ -e "$RE_TARGET" ]; then
        echo Success. "$RE_TARGET" is generated.
    else
        echo ERROR: Target not found.
        exit 1
    fi
else
    cd ..
    echo ERROR: Zipping failed.
    exit 12
fi

exit 0

################################################################################
