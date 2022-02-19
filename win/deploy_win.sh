# Run inside build dir
# pack ./deploy/ as distrubution

export PATH=$PATH:/e/Qt/Tools/mingw810_64/bin/:/e/Qt/5.15.2/mingw81_64/bin/

windeployqt.exe ./app/mpz.exe --dir deploy/
cp ./app/mpz.exe deploy/

# gh release upload 1.0.19 ~/Desktop/mpz-1.0.19-win64-dynamic.zip
