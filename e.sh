#!/bin/bash

bspc rule -a '*' -o state=floating
./build/linux/$1
