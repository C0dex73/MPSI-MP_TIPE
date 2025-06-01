#!/bin/bash
#   Script used to get all nodes from a folder
#   Nodes returned can be either executable nodes (-E) or library nodes (-L), default is -E
#   $1 is the folder to search for nodes in
#   Example : ./scripts/getnodes ./src/
#   will return all nodes that contains source code for an executable in the directory ./src/
#   Whereas : ./scripts/getnodes -L ./src2/
#   will return all nodes that contains source code for a library in the directory ./src2/
#
#   NOTE : The term "node" refers to a subfolder containing source code for either executables (executable nodes) or libraries (library nodes)

if [ 2 -gt $# ]; then
    echo "Error: Invalid number of arguments"
    exit 2
fi

nodes=""
condition=""
case $1 in
    "-E")
        condition="main.c";;
    "-L");;
    *)
        echo "Error: Invalid argument: $1"
        exit 2;;
esac

for dir in $@; do
    if [ $dir = $1 ]; then continue; fi
    if ! [ -d $dir ] || ! [ "$(ls -A $dir)" ]; then
        echo "Warning: directory $dir does not exists, skipping..."
        continue
    fi
    for subdir in $(echo $dir/*/); do
        if [ "$(ls -l $subdir | grep main.c | tail -c 7)" = "$condition" ]; then
            nodes="$nodes $subdir"
        fi
    done
done

echo $nodes