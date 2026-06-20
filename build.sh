#!/bin/bash
set -e

clear
cmake . -B build
cd build
make
cp main ../main
cd ..

rm ./*.csv