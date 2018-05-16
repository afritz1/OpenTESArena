#!/bin/bash

wget https://github.com/Mindwerks/wildmidi/archive/master.zip
unzip master.zip
pushd wildmidi-master && mkdir build && pushd build && cmake .. && make && sudo make install && popd && popd
