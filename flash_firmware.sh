install_commander()
{
    commander_root_path=$1
    echo -e "\n*** Downloading Simplicity Commander ***\n"
    mkdir $commander_root_path
    cd $commander_root_path
    wget https://www.silabs.com/documents/public/software/SimplicityCommander-Linux.zip

    echo -e "\n*** Installing Simplicity Commander ***\n"
    unzip *.zip
    mv SimplicityCommander-Linux/* ./
    rmdir SimplicityCommander-Linux
    tar -xvf *.tar.bz

    echo -e "\n*** Removing .tar.bz and .zip ***\n"
    rm -f *.tar.bz
    rm -f *.zip*

    cd ../
}

# Check arguments
if [ $# -lt 1 ]
then
    printf "Usage $0 <firmware_file(.s37)>\n"
    exit 1
fi

commander_root_path=commander
commander=$commander_root_path/commander/commander

# Check if commander exists, if not, install it
if [[ ! -f $commander ]]
then
    echo "$commander does not exist on your filesystem."
    install_commander $commander_root_path
fi

# Flash firmware with bootloader
bootloader=receiver_files/bootloader-storage-spiflash-single.s37
firmware=$1

$commander flash $bootloader $firmware --device efr32bg22