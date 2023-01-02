#!/bin/bash
sudo apt-get update
sudo apt-get -y install wget
sudo apt-get -y install gcc
sudo apt-get -y install srecord
sudo apt-get -y install build-essential
sudo apt-get -y install libgmp-dev libmpfr-dev libmpc-dev
sudo apt-get -y install texinfo
sudo apt-get -y install libncurses5-dev
sudo apt-get -y install flex
sudo apt-get -y install bison
sudo apt-get -y install lxterminal
cd /tmp
wget http://www.lwtools.ca/releases/lwtools/lwtools-4.18.tar.gz
tar xvf lwtools-4.18.tar.gz
cd /tmp/lwtools-4.18/
make
sudo make install
cd /tmp/lwtools-4.18/extra
sudo cp ar /usr/local/bin/m6809-unknown-ar
sudo cp as /usr/local/bin/m6809-unknown-as
sudo cp ld /usr/local/bin/m6809-unknown-ld
sudo ln -s /bin/true /usr/local/bin/m6809-unknown-nm
sudo ln -s /bin/true /usr/local/bin/m6809-unknown-objdump
sudo ln -s /bin/true /usr/local/bin/m6809-unknown-ranlib
cd /tmp
wget http://mirror.koddos.net/gcc/releases/gcc-4.6.4/gcc-4.6.4.tar.bz2 
tar jxvf gcc-4.6.4.tar.bz2
cd gcc-4.6.4 
patch -p1 < /tmp/lwtools-4.18/extra/gcc6809lw-4.6.4-9.patch
cd /tmp
mkdir gcc-build
cd gcc-build
../gcc-4.6.4/configure --enable-languages=c --target=m6809-unknown --program-prefix=m6809-unknown- --enable-obsolete --srcdir=../gcc-4.6.4 --disable-threads --disable-nls --disable-libssp --prefix=/usr/local --with-as=/usr/local/bin/m6809-unknown-as --with-ld=/usr/local/bin/m6809-unknown-ld --with-ar=/usr/local/bin/m6809-unknown-ar
make 
sudo make install
cd /tmp
git clone -b m6809-7.6 https://www.6809.org.uk/git/binutils-gdb.git
cd binutils-gdb
./configure --target=m6809-unknown --program-prefix=m6809-unknown- --prefix=/usr/local --disable-werror --with-system-zlib --disable-etc --disable-nls --with-python=no 
make
sudo make install