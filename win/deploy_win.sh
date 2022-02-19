# Run inside build dir
# pack ./mpz/ as distrubution

export PATH=$PATH:/e/Qt/Tools/mingw810_64/bin/:/e/Qt/5.15.2/mingw81_64/bin/

windeployqt.exe ./app/mpz.exe --dir mpz/
cp ./app/mpz.exe mpz/
