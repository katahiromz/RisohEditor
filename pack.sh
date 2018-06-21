#!/bin/bash
# pack.sh --- RisohEditor deploy script
################################################################################
# RisohEditor --- Another free Win32 resource editor
# Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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
RE_VERSION=4.9.8
RE_BIN_DIR="build/re-$RE_VERSION-bin"
RE_FILES="README.txt READMEJP.txt LICENSE.txt Standardize.md HYOJUNKA.txt src/resource.h build/RisohEditor.exe mcdx/MESSAGETABLEDX.md"
RE_TARGET="build/re-$RE_VERSION-bin.zip"

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

echo Copying Stage 1...
if cp $RE_FILES "$RE_BIN_DIR"; then
    echo Copying Stage 2...
    if cp -r data "$RE_BIN_DIR"; then
        echo Copying Stage 3...
        if cp build/mcdx.exe "$RE_BIN_DIR/data/bin"; then
            echo Copying Stage 4...
            mkdir "$RE_BIN_DIR/OLE"
            if cp -f src/MOleCtrl.hpp include/MWindowBase.hpp "$RE_BIN_DIR/OLE"; then
                echo Copying Stage 5...
                mkdir "$RE_BIN_DIR/MyWndCtrl"
                if cp -f "MyWndCtrl/MyWndCtrl.cpp" "MyWndCtrl/MWindowBase.hpp" "MyWndCtrl/CMakeLists.txt" "$RE_BIN_DIR/MyWndCtrl"; then
                    echo Copying Stage 6...
                    if cp -f build/MyWndCtrl.dll "$RE_BIN_DIR/MyWndCtrl"; then
                        echo Copying Stage 7...
                        mkdir "$RE_BIN_DIR/DlgInit"
                        if cp -f "src/DlgInit.h" "$RE_BIN_DIR/DlgInit"; then
                            echo Zipping...
                            cd build
                            if zip -9 -r -q "re-$RE_VERSION-bin.zip" "re-$RE_VERSION-bin"; then
                                cd ..
                                if [ -e "$RE_TARGET" ]; then
                                    echo Success. "$RE_TARGET" was generated.
                                else
                                    echo ERROR: Target not found.
                                    exit 13
                                fi
                            else
                                cd ..
                                echo ERROR: Zipping failed.
                                exit 12
                            fi
                        else
                            echo ERROR: Copying Stage 7 failed.
                            exit 11
                        fi
                    else
                        echo ERROR: Copying Stage 6 failed.
                        exit 10
                    fi
                else
                    echo ERROR: Copying Stage 5 failed.
                    exit 9
                fi
            else
                echo ERROR: Copying Stage 4 failed.
                exit 8
            fi
        else
            echo ERROR: Copying Stage 3 failed.
            exit 7
        fi
    else
        echo ERROR: Copying Stage 2 failed.
        exit 6
    fi
else
    echo ERROR: Copying Stage 1 failed.
    exit 5
fi

exit 0

################################################################################
