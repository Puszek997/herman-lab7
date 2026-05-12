#!/usr/bin/env bash

give_info () {
    sleep $3
    printf "%s\n" $1
    sleep $3
    printf "%s" $1
}

SELF=$1
LOVE=$2
DELAY=${3:-"1"}
TIMEOUT=$((${DELAY}*2))

give_info $SELF $LOVE $DELAY | ./base_client.sh ${SELF} -w ${TIMEOUT}