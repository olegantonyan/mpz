#!/bin/bash

SRC_DIR=$(cd `dirname $0` && pwd)
TMP_DIR=$(mktemp -d -t mpz-aur-$(date +%Y-%m-%d-%H-%M-%S)-XXXXX)
cd $TMP_DIR

echo -e "TMP_DIR:\t$TMP_DIR"

cat << EOF > Dockerfile
FROM archlinux:latest

RUN pacman -Sy base-devel --noconfirm
RUN useradd -m build

CMD ["su", "-", "build", "-c", "makepkg --printsrcinfo ~/build/PKGBUILD"]
EOF

docker build -t arch-makepkg .

git clone ssh://aur@aur.archlinux.org/mpz.git
cd mpz
cp $SRC_DIR/PKGBUILD .
docker run -it --rm -v $SRC_DIR:/home/build arch-makepkg > .SRCINFO
git add . --all && git commit -m "update AUR package" && git push
