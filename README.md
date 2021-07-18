OsmAnd Core
==============

Multi-platform core for OsmAnd project

Installation on Ubuntu 
=====================

/!\ outdated information, see here instead : https://docs.osmand.net/en/main@latest/development/build-osmand/setup-the-dev-environment

1. Fresh install Ubuntu 14.04 LTS
2. Install repo
 * mkdir ~/bin
 * PATH=~/bin:$PATH
 * curl https://storage.googleapis.com/git-repo-downloads/repo > ~/bin/repo
 * chmod a+x ~/bin/repo
3. Install software
 * sudo apt-get install mc git build-essential libfontconfig1-dev freeglut3-dev libxrender-dev libxrandr-dev libxi-dev openjdk-7-jdk
 * sudo add-apt-repository ppa:ubuntu-toolchain-r/test
 * sudo apt-get update
 * sudo apt-get install gcc-4.9 gcc-4.9-multilib g++-4.9 g++-4.9-multilib
4. Use Bash not Dash
 * sudo dpkg-reconfigure dash
5. install cmake
 * wget -c https://cmake.org/files/v3.6/cmake-3.6.2.tar.gz
 * tar -xvf cmake-3.6.2.tar.gz
 * cd cmake-3.6.2
 * ./configure && make && sudo make install
6. Configure git; Get OsmAnd sources
 * git config --global user.email "noone@example.com"
 * git config --global user.name "No One"
 * mkdir project
 * cd project
 * repo init -u https://github.com/osmandapp/OsmAnd-manifest -m readonly.xml
 * repo sync
 * ./build/amd64-linux-gcc.sh
 * cd baked/amd64-linux-gcc.make
 * make

Before building android wrapper!
================================
Install clang toolchains:
cd ANDROID_NDK/build/tools
./make_standalone_toolchain.py --arch arm --api 14 --stl=libc++ --force --install-dir ../../toolchains/arm-linux-androideabi-clang/prebuilt/linux-x86_64
./make_standalone_toolchain.py --arch arm64 --api 21 --stl=libc++ --force --install-dir ../../toolchains/aarch64-linux-android-clang/prebuilt/linux-x86_64
./make_standalone_toolchain.py --arch x86 --api 14 --stl=libc++ --force --install-dir ../../toolchains/x86-clang/prebuilt/linux-x86_64
