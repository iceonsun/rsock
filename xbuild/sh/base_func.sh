#!/usr/bin/env bash

function get_os {
    local os=$(uname)

    if [[ "${os}" == *Darwin* ]]; then
        echo "using MacOs" > /dev/stderr
        return 1
    elif [[ "${os}" == *Linux* ]]; then
        echo "using Linux" > /dev/stderr
        return 2
    else
        echo "Don't know what system you are using." > /dev/stderr
        return 0
    fi
    return 0

}

function get_num_core {
    local num_core=1
    get_os
    local os=$?
    # get number of cores
    if [ ${os} -eq 1 ]; then     # mac
        num_core=$(sysctl -n hw.ncpu)
    elif [ ${os} -eq 2 ]; then  # linux
        num_core=$(grep -c '^processor' /proc/cpuinfo)
    fi
    return ${num_core}
}

function strindex {
  x="${1%%$2*}"
  [[ "$x" = "$1" ]] && echo -1 || echo "${#x}"
}