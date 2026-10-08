#!/bin/bash

if [ ! -f "otesa" ]; then
    echo "'otesa' binary not found in the current directory."
    exit 1
fi

DISTRIBUTED=( libopenal libSDL2 libsndio libvulkan libWildMidi )

if [ ! -d "libs" ]
then
	mkdir libs
fi

for file in "${DISTRIBUTED[@]}"
do
    path=$(ldd otesa | grep -w "$file" | awk '{print $3}')
    if [ -f "$path" ]; then
        cp -L "$path" "libs/"
    fi
done
