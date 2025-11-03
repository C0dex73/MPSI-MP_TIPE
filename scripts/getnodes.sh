#!/bin/bash
#   Script used to get all nodes from a folder
#   Nodes returned can be either executable nodes (-E), library nodes (-L), or other nodes (-O)
#   $2 is the folder to search for nodes in
#   Example : ./scripts/getnodes.sh -E ./src/
#   will return all nodes that contains source code for an executable in the directory ./src/
#   Whereas : ./scripts/getnodes.sh -L ./src2/
#   will return all nodes that contains source code for a library in the directory ./src2/
#
#   NOTE :  The term "node" refers to a subfolder of $2
#
#   CLASSIFICATION :
#           Nodes are classified "other" when they contain a makefile of their own at their root (protocol for other nodes is to use this makefile instead of the one for libraries or executables).
#           Nodes are classified "executable" when they contain a file named main, whatever its extension is.
#           Leftover nodes are classified "library".
#
#   HELP : If $1 is -h, displays this message


# -h to diplay help message
if [ "-h" == "$1" ]; then
    head -n 19 $0 | tail -n 18 | cut -c 5-
    exit 0
fi

# checks if there is 2 args (filter and folder)
if ! [ 2 -eq $# ]; then
    echo "Error: Invalid number of arguments."
    exit 2
fi

# checks if filter is valid
if  ! [[ "-E -L -O" =~ ( |^)$1( |$) ]]; then
    echo "Error: Unknown argument '$1.'"
    exit 2
fi

# checks if search folder exists
if ! [ -d $2 ]; then
    echo "Error: $2 is not a valid directory."
    exit 2
fi

#empty the node vars
Enodes=""
Lnodes=""
Onodes=""

for dir in $(echo $2/*/); do
    # if makefile, then other node
    if [ -f $dir/Makefile ] || [ -f $dir/makefile ] || [ -f $dir/GNUmakefile ]; then
        Onodes="$Onodes $dir"
    elif [ -n "$(ls $dir | grep -w main)" ]; then
        Enodes="$Enodes $dir"
    else
        Lnodes="$Lnodes $dir"
    fi
done

case $1 in
    "-L")
        echo $Lnodes ;;
    "-E")
        echo $Enodes ;;
    "-O")
        echo $Onodes ;;
    *)
        echo "Critical Error : Unexpected branching"
        exit 1 ;;
esac

exit 0