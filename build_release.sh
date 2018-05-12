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

    [[ -d ${DIR} ]] || echo "doesnot exists"

    [[ -d ${DIR} ]] || mkdir ${DIR}
    if [ $? -ne 0 ]; then
        echo "cannot create directory ${BUILD_DIR}/build"
        exit 1
    fi

    cd ${DIR}
    rm rsock*.tar.gz
    rm rsock*.zip
    local VERSION_FILE=VERSION.txt
    local SUM_FILE=sum.txt

    echo "build on `date -u`" > ${SUM_FILE}

    echo "build on `date -u`" > ${VERSION_FILE}
    echo "commit head: $(git rev-parse HEAD)" >> ${VERSION_FILE}

    get_num_core
    local num_core=$?
    for chainfile in ${BUILD_DIR}/cmake/Darwin_x86_64.toolchain.cmake ${BUILD_DIR}/cmake/Linux_x86_64.toolchain.cmake; do
        echo "toolchain file: ${chainfile}"
        local filename=$(basename "$chainfile")

        local sub_arch_dir=$(echo ${filename} |cut -d'.' -f 1)
        if [ -d ${sub_arch_dir} ]; then
            [[ -f CMakeCache.txt ]] && rm CMakeCache.txt
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
            cp ../${VERSION_FILE} .
            if [ ${SYSNAME} = "Darwin" ]; then
                TAR_FILE="rsock-${SYSNAME}-${ARCH}-${BUILD_VERSION}.zip"
                zip "${TAR_FILE}" "server_rsock_${SYSNAME}" "client_rsock_${SYSNAME}" ${VERSION_FILE}
            else
                tar -czf "${TAR_FILE}" "server_rsock_${SYSNAME}" "client_rsock_${SYSNAME}" ${VERSION_FILE}
            fi
            rm ${VERSION_FILE}
            echo
            local sum=$(shasum ${TAR_FILE})
            echo ${sum} | tee -a ../${SUM_FILE}
            echo
            mv ${TAR_FILE} ..
        fi

        cd -
    done

    cd ${TOP_DIR}
}

build_cross_binaries
