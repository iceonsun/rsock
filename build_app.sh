#!/bin/bash

source ./xbuild/sh/base_func.sh

declare -a CMD_LIST=("g++" "cmake" "make")
declare -a LIB_LIST=(uv net pcap)  # be modified in function install_cmd_and_libs
declare -a INSTALLED_LIB_LIST=(libuv libnet libpcap)  # be modified in function install_cmd_and_libs

OS=1    #linux by default

function install_cmd_and_libs {
    local CMD="apt-get"
    if which apt-get; then
        INSTALLED_LIB_LIST=(libuv1-dev libnet-dev libpcap-dev)
        CMD="apt-get"
    elif which yum; then
        INSTALLED_LIB_LIST=(libuv1-devel libnet-devel libpcap-devel)
        CMD="yum"
    elif which brew; then
        INSTALLED_LIB_LIST=(libuv libnet libpcap)     # macos
        CMD="brew"
    else
        echo "You have to install ${CMD_LIST[@]} and libuv libnet libpcap manualy."
        exit 1
    fi

    for i in "${CMD_LIST[@]}"
    do
        which ${i}
        if [ $? -ne 0 ]; then
            echo "install command $i"
            eval "${CMD} install ${i}"
            if [ $? -ne 0 ]; then
                echo "failed to install $i"
                exit
            fi
        else
            echo "skip to install $i. already installed"
        fi
    done

    for i in "${!INSTALLED_LIB_LIST[@]}"
    do
        local name4ck=${LIB_LIST[$i]}
        local name4ins=${INSTALLED_LIB_LIST[$i]}
        echo "name4ck: ${name4ck}, name4ins: ${name4ins}"
        local ok=1
        if [ ${OS} -eq 2 ]; then    # linux library check
            ldconfig -p |grep ${name4ck}
            if [ $? -ne 0 ]; then
                ok=0
            fi
        elif [ ${OS} -eq 1 ]; then    # mac library check
            ld "-l${name4ck}" > err.log 2>&1
            local ret=$(cat err.log)
            rm err.log
            if [[ ${ret} == *"library not found for"* ]]; then
#            if [[ *"library not found for"* == ${ret} ]]; then # wrong!!! cannot change order
                echo "no library!!!"
                ok=0
            fi
         else
            echo "unsupported platform. You should build it manually."
            exit 1
        fi

        if [ ${ok} -eq 0 ]; then
            echo "install library ${name4ins}"
            eval "${CMD} install ${name4ins}"
            if [ $? -ne 0 ]; then
                echo "failed to install $i"
                exit $3
            fi
        else
            echo "skip to install ${name4ins}. already installed"
        fi
    done

}

function run_build {
    local TOP_DIR=$(pwd)
    local DIR="${TOP_DIR}/build"
    [[ -d ${DIR} ]] || mkdir ${DIR}
    if [ $? -ne 0 ]; then
        echo "failed to create directory ${DIR}"
        exit 1
    fi

    cd ${DIR}
    make clean

    get_num_core
    local num_core=$?

    cmake ${TOP_DIR} ../ && make "-j$num_core"
    if [ $? -eq 0 ]; then
        echo "build successful."
    else
        echo "build failed."
    fi
}

function check_os {
    get_os
    OS=$?
    if [ ${OS} -eq 0 ]; then
        echo "Don't know what system you are using. You have to install ${CMD_LIST} ${INSTALLED_LIB_LIST} by your self"
    fi
}

check_os
install_cmd_and_libs
run_build