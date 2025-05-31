#!/bin/bash
#   script used to create a node
#   -E, -B and -L are used to specify the node type (executable, binary or library), default is executable
#   the name of the node follow the type directive
#   -S is used to specify the folder in which the nodes shall be created, default is ./
#   Caution : -S./dir/ is not the same as -S ./dir/
#   
#
#
#   create-node simulation -S                                            ->      creates a executable node named "simulation" in ./
#   create-node -E simulation_console -S./src/ simulation_windows        ->      creates executable nodes named "simulation_console" in ./ and "simulation_windows" in ./src/
#   create-node -B -S./worlds/ -L dimension1 dimension2 -E universe         ->      creates binary nodes named "dimension1" and "dimension2" as well as a executable node named "universe" in ./worlds/

execs=""
bins=""
libs=""
dir="./"
type=0 # 0->exec | 1->lib

for cmd in $@; do
    case $cmd in
    "-E")
        type=0;;
    "-L")
        type=1;;
    "-B")
        type=2;;
    "-S"*)
        dir=${cmd/#-S};;
    *)
        case $type in
        2)
            bins="$bins $dir/$cmd";;
        1)
            libs="$libs $dir/$cmd";;
        *)
            execs="$execs $dir/$cmd";;
        esac
    esac
done

if [ "$libs" == "" ] && [ "$execs" == "" ] && [ "$bins" == "" ]; then
    echo "Error : expected a node name"
    exit 2
fi

for node in $execs; do
    mkdir -p "$node"
    touch "$node/main.c"
    touch "$node/main.h"
    touch "$node/gcc.flags"
    echo "Created executable node : $node"
done

for node in $bins; do
    mkdir -p "$node/bin"
    touch "$node/bin/dependencies.lnk"
    touch "$node/$(basename $node).h"
    echo "Created binary node : $node"
done

for node in $libs; do
    mkdir -p "$node"
    touch "$node/$(basename $node).c"
    touch "$node/$(basename $node).h"
    touch "$node/gcc.flags"
    echo "Created library node : $node"
done

echo "Operations successfully terminated !"