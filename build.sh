#!/bin/bash
set -e

clear
cmake . -B build
cd build
make -j
cp NIR ../NIR
cd ..

rm -f ./*.csv
