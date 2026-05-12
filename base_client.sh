#!/usr/bin/env bash

NAME=${1:-"NONAME"}
shift
set -x
nc "$@" -U Laurenty | awk -v var="${NAME}" '{print var ": " $0}'
set +x
