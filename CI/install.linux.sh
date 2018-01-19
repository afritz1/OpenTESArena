#!/bin/sh

wget https://github.com/Mindwerks/wildmidi/archive/wildmidi-0.4.0.tar.gz
tar -xzvf wildmidi-0.4.0.tar.gz
pushd wildmidi-wildmidi-0.4.0 && mkdir build && pushd build && cmake .. && make && sudo make install && popd && popd