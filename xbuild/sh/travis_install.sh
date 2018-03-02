#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$DIR/base_func.sh"
if [ $? -ne 0 ]; then
    echo "the script can only be called in root directory of project"
    return 1
fi

function build_on_travis {
    mkdir build
    cd build
    pwd
    OS=$(uname)

    ARCH=$(uname -m)
    if [ $ARCH != "x86_64" ]; then
        echo "only x86_64 supported"
    fi

    local core=$(get_num_core)
    cmake -DCMAKE_TOOLCHAIN_FILE="../xbuild/cmake/travis_${OS}_x86_64.cmake" ..
    make "-j$core"
    if [ $? -eq 0 ];then
        echo "build success"
        echo "check library dependency ..."
        for f in *_rsock_${OS}; do

            if which readelf; then
                readelf -d ${f}
                readelf -d ${f} |grep '++'

            elif which otool; then
                otool -L ${f}
                otool -L ${f} |grep '++'
            fi
            if [ $? -eq 0 ]; then
                echo "library dependency not resolved"
                echo "build failed"
                exit 1
            fi
        done
    else
        echo "build failed"
        exit 1
    fi
}

build_on_travis


