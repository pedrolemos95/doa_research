COMMANDER=./SimplicityCommander-Linux/commander/commander

if [ $# -lt 1 ]
then
    printf "Usage $0 <binary_file>\n"
    exit 1
fi

binary_file=$1

# Flash firmware into device
$COMMANDER flash $binary_file