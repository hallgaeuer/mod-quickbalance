#!/usr/bin/env bash

MOD_QUICKBALANCE_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )/" && pwd )"

source $MOD_QUICKBALANCE_ROOT"/conf/conf.sh.dist"

if [ -f $MOD_QUICKBALANCE_ROOT"/conf/conf.sh" ]; then
    source $MOD_QUICKBALANCE_ROOT"/conf/conf.sh"
fi
