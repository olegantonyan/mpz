#!/usr/bin/env bash

flatpak-builder --user --install --force-clean build-flatpak `dirname $0`/org.mpz_player.mpz.yml
flatpak run org.mpz_player.mpz
flatpak uninstall -y org.mpz_player.mpz
