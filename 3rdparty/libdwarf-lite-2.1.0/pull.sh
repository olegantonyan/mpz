#!/bin/bash

echo "Removing files"
rm -rfv cmake src CMakeLists.txt COPYING
echo "Fetching"
git clone https://github.com/davea42/libdwarf-code.git
cd libdwarf-code
# git checkout "6216e185863f41d6f19ab850caabfff7326020d7" # v0.8.0
# git checkout "8b0bd09d8c77d45a68cb1bb00a54186a92b683d9" # v0.9.0
# git checkout "8cdcc531f310d1c5ae61da469d8056bdd36b77e7" # v0.9.1 + cmake fixes
# git checkout "5e43a5ab73cb00c8a46660b361366a8c9c3c93c9" # v0.9.2
# git checkout "45ef8e2763f65c31b27cc38bed197b84dc1441d4" # v0.10.2
# git checkout "285d9d34f3e9f56cc1c487d0055f6dc54a9c54a1" # v0.11.0
# git checkout "909af3e46b68335df6c4a901ddd256ffa0d193d2" # v0.11.1
# git checkout "78232149dfa4740387aedbc0a5bb4e3ba111258e" # v0.12.0
# git checkout "be6d4ca64e41698514dc2355f9ae75070aed119f" # v2.0.0
git checkout "2fb65851ff757eb8f6f6777954bdfa4bced2911e" # v2.1.0
cd ..
echo "Copying files"
mkdir -p src/lib
mv -v libdwarf-code/CMakeLists.txt .
mv -v libdwarf-code/COPYING .
mv -v libdwarf-code/cmake .
mv -v libdwarf-code/src/lib/libdwarf src/lib
echo "Deleting cloned repo"
rm -rf libdwarf-code
echo "Cleaning up src/lib/libdwarf"
cd src/lib/libdwarf
rm -rfv ChangeLog* CHANGES CODINGSTYLE NEWS
cd ../../..
echo "Patching"
patch < patches/CMakeLists.patch
echo "Done"
