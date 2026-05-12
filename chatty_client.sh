#!/usr/bin/env bash

give_info () {
    sleep $3
    printf "%s\n" $1
    sleep $3
    printf "%s\n" $2
    sleep $3
    printf "Hello!\n"
    i=1
    while (( i < 6 )); do
        sleep $3
        printf "How are you?\n"
        i=$(($i+1))
    done
}

SELF=$1
LOVE=$2
DELAY=${3:-"1"}
TIMEOUT=$((${DELAY}*2))

give_info $SELF $LOVE $DELAY | ./base_client.sh ${SELF} -w ${TIMEOUT}