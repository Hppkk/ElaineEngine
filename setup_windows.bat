@echo off
git submodule update --init --recursive
cmake . -B Build
pause