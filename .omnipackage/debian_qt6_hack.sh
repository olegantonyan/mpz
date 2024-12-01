#!/usr/bin/env bash

set -xEeuo pipefail


# https://bugs.launchpad.net/ubuntu/+source/qtchooser/+bug/1964763
ln -s /usr/bin/qmake6 /usr/bin/qmake
