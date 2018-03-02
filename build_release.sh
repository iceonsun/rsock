#!/usr/bin/env bash

source ./xbuild/sh/base_func.sh

function build_cross_binaries {
    which shasum
    if [ $? -ne 0 ]; then
        echo "you have to install shasum first"
        exit 2
    fi

    local TOP_DIR=$(pwd)
    local BUILD_DIR=$(pwd)/xbuild
    local BUILD_VERSION=$(date -u +%Y%m%d)

    cd ${BUILD_DIR}
    local DIR="build"

    if [ -d ${DIR} ]; then
        rm rsock*.zip
    else
        mkdir mkdir ${DIR}
        if [ $? -ne 0 ]; then
            echo "cannot create directory ${BUILD_DIR}/build"
            exit 1
        fi
    fi

    cd ${DIR}

    echo "build on `date -u`" > sum.txt

    echo "build on `date -u`" > VERSION.txt
    echo "commit head: $(git rev-parse HEAD)" >> VERSION.txt

    get_num_core
    local num_core=$?
    for chainfile in ${BUILD_DIR}/cmake/*.toolchain.cmake; do
        echo "toolchain file: ${chainfile}"
        local filename=$(basename "$chainfile")

        local sub_arch_dir=$(echo ${filename} |cut -d'.' -f 1)
        if [ -d ${sub_arch_dir} ]; then
            make clean
            else
                mkdir ${sub_arch_dir}
                if [[ $? -ne 0 ]]; then
                echo "cannot create directory ${sub_arch_dir}"
                exit 1
            fi
        fi

        local SYSNAME=$(echo ${filename} |cut -d'_' -f 1)

#        local archpos=$[${#sub_arch_dir} - ${#sysname} - 1]
        local archpos=$[1 + ${#SYSNAME} - ${#sub_arch_dir}] # index backwards

        local ARCH=${sub_arch_dir:${archpos}}

        cd ${sub_arch_dir}
        cmake -DCMAKE_TOOLCHAIN_FILE=${chainfile} ${TOP_DIR}
        make -j${num_core}

        if [ $? -ne 0 ]; then
            echo "failed to make target $sub_arch_dir"
            exit 3
        else
            local TAR_FILE="rsock-${SYSNAME}-${ARCH}-${BUILD_VERSION}.tar.gz"
            cp ../VERSION.txt .
            tar -czf "${TAR_FILE}" "server_rsock_${SYSNAME}" "client_rsock_${SYSNAME}" VERSION.txt
            rm VERSION.txt
            echo
            local sum=$(shasum ${TAR_FILE})
            echo ${sum}
            echo ${sum} >> ../sum.txt
            echo
            mv ${TAR_FILE} ..
        fi

        cd -
    done

    rm VERSION.txt

    cd ${TOP_DIR}
}

build_cross_binaries