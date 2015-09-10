#!/bin/bash

# This is a script that facilitates the set-up of a working compilation and running
# environment on a fresh Ubuntu (tested on 15.04) machine.
# /etc/environment is used to establish environment variables also in non-interactive sessions.
# Partially sourced from:
#  - http://sysads.co.uk/2014/05/install-open-mpi-1-8-ubuntu-14-04-13-10/
#  - http://particlephysicsandcode.com/2013/03/11/installing-boost-1-52-ubuntu-12-04-fedora/

#######
# PRE #
#######
apt-get update
apt-get upgrade -y

apt-get install htop vim openssh-server build-essential clang cmake python-dev autotools-dev libicu-dev libibnetdisc-dev -y

export CC=$(which clang)
export CXX=$(which clang++)

echo "CC=${CC}" >> /etc/environment
echo "CXX=${CXX}" >> /etc/environment

###########
# OPENMPI #
###########
cd /tmp/
wget -O openmpi-1.8.8.tar.gz http://www.open-mpi.org/software/ompi/v1.8/downloads/openmpi-1.8.8.tar.gz
tar xvzf openmpi-1.8.8.tar.gz
cd openmpi-1.8.8
./configure
make
make install
cd ..

export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/lib/openmpi/:/usr/local/lib/"
echo "LD_LIBRARY_PATH=\"/usr/local/lib/openmpi/:/usr/local/lib/\"" >> /etc/environment

#########
# BOOST #
#########
cd /tmp/
wget -O boost_1_58_0.tar.gz http://sourceforge.net/projects/boost/files/boost/1.58.0/boost_1_58_0.tar.gz/download
tar xvzf boost_1_58_0.tar.gz
cd boost_1_58_0

install_dir="/usr/local/boost-1.58.0"
./bootstrap.sh --prefix=${install_dir}

user_config_file=$(find $PWD -name project-config.jam)
echo "using mpi ;" >> ${user_config_file}

./b2 --with=all -j $(nproc) cxxflags="-std=c++11" --target=shared,static install

echo "$install_dir/lib" >> /etc/ld.so.conf.d/boost-1.58.0.conf
ldconfig -v
cd ..

export BOOST_ROOT=${install_dir}
echo "BOOST_ROOT=$BOOST_ROOT" >> /etc/environment

########
# POST #
########
service ssh start
cd
# the following prepends the path with ours (# is the sed separator instead of /)
sed -i -- "s#PATH=\"#PATH=\"$HOME:$HOME/out:$HOME/mpi/out:#g" /etc/environment
echo "source /etc/environment" >> ~/.bashrc

read -p "The computer will now reboot to establish correct environemnt variables. " foo
reboot
