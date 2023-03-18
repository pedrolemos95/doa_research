printf "**** Fazendo o download do Simplicity Commander ****\n"
wget https://www.silabs.com/documents/public/software/SimplicityCommander-Linux.zip
printf "**** Instalando o Simplicity Commander ****\n"
unzip *.zip
cd SimplicityCommander-Linux
tar -xvf *.tar.bz
cd ../
rm *.zip