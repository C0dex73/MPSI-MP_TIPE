#!/bin/sh
#   Script used to get all nodes from a folder
#   Nodes returned can be either executable nodes ($1 = "EXEC")
#   Or Library nodes ($1 = "LIB")
#   $1 is the folder to search for nodes in
#   Example : ./scripts/getnodes "EXEC" ./src/
#   will return all nodes that contains source code for an executable in the directory ./src/
#
#   NOTE : The term "node" refers to a subfolder containing source code for either executables (executable nodes) or libraries (library nodes)

if [ 2 -gt $# ]; then
    echo "Error: Invalid number of arguments"
    exit 2
fi

nodes=""
condition=""
case $1 in
    "EXEC")
        condition="main.c";;
    "LIB");;
    *)
        echo "Error: Invalid argument: $1"
        exit 2;;
esac

for dir in $@; do
    if [ $dir = $1 ]; then continue; fi
    if ! [ -d $dir ]; then
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