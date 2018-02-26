#!/bin/bash

RE_VERSION=3.5
RE_BIN_DIR="build/re-$RE_VERSION-bin"
RE_FILES="README.txt READMEJP.txt LICENSE.txt src/resource.h build/RisohEditor.exe"
RE_TARGET="build/re-$RE_VERSION-bin.zip"

if [ ! -d build ]; then
    echo ERROR: the build directory doesn't exists.
    exit 1
fi

if [ ! -d data ]; then
    echo ERROR: the data directory doesn't exists.
    exit 2
fi

if [ ! -e build/RisohEditor.exe ]; then
    echo ERROR: build/RisohEditor.exe doesn't exists.
    exit 3
fi

if [ ! -e build/mcdx.exe ]; then
    echo ERROR: build/mcdx.exe doesn't exists.
    exit 4
fi

if [ -e "$RE_BIN_DIR" ]; then
    echo Deleting "$RE_BIN_DIR"...
    rm -fr "$RE_BIN_DIR"
    mkdir "$RE_BIN_DIR"
else
    mkdir "$RE_BIN_DIR"
fi

echo Copying No.1...
if cp $RE_FILES "$RE_BIN_DIR"; then
    echo Copying No.2...
    if cp -r data "$RE_BIN_DIR"; then
        echo Copying No.3...
        if cp build/mcdx.exe "$RE_BIN_DIR/data/bin"; then
            echo Zipping...
            if zip -9 -r -q "$RE_TARGET" "$RE_BIN_DIR"; then
                if [ -e "$RE_TARGET" ]; then
                    echo Success. "$RE_TARGET" was generated.
                else
                    echo ERROR: Target not found.
                    exit 9
                fi
            else
                echo ERROR: Zipping failed.
                exit 8
            fi
        else
            echo ERROR: Copying No.3 failed.
            exit 7
        fi
    else
        echo ERROR: Copying No.2 failed.
        exit 6
    fi
else
    echo ERROR: Copying No.1 failed.
    exit 5
fi

exit 0
