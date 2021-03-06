#!/usr/bin/env bash

set -e


sudo apt-get install build-essential libboost-serialization-dev \
libboost-filesystem-dev \
libeigen-dev \
autoconf libtool pkg-config

# shellcheck disable=SC2034
# shellcheck disable=SC2046
install_dir=$(dirname $(pwd))/3rd

if [ ! -d "${install_dir}" ]; then
    mkdir -p "${install_dir}"
fi

cd /tmp && rm -rf gcem
echo "Install dependencies: GCE-Math to ${install_dir} ..."
git clone https://github.com/kthohr/gcem.git
cd gcem
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=${install_dir}
make && make install
cd /tmp && rm -rf gcem
echo "DONE!"

cd /tmp && rm -rf stats
echo "Install dependencies: StatsLib to ${install_dir} ..."
git clone https://github.com/kthohr/stats.git
cd stats
# no need to compile since it is a header-only library
cp -r include "${install_dir}"
cd /tmp && rm -rf stats


cd /tmp && rm -rf stlcache
echo "Install dependencies: STL:Cache to ${install_dir} ..."
git clone https://github.com/akashihi/stlcache.git
cp -r stlcache/include "${install_dir}"
cd /tmp && rm -rf stlcache


cd /tmp && rm -rf orderd-map
git clone https://github.com/Tessil/ordered-map.git
cp -r orderd-map/include "${install_dir}"
cd /tmp && rm -rf orderd-map

cd /tmp && rm -rf fmt
echo "Install dependencies: fmtlib to ${install_dir} ..."
git clone https://github.com/fmtlib/fmt.git
mkdir fmt/build && cd fmt/build
cmake .. -DCMAKE_INSTALL_PREFIX=${install_dir}
make -j$(nproc)
make install
cd /tmp && rm -rf fmt

cd /tmp && rm -rf xxHash
git clone https://github.com/Cyan4973/xxHash.git
mkdir xxHash/build && cd xxHash/build
cmake .. -DCMAKE_INSTALL_PREFIX=${install_dir}
make -j$(nproc)
make install
cd /tmp && rm -rf xxHash


cd /tmp && rm -rf grpc
git clone https://github.com/grpc/grpc.git
cd grpc
git submodule update --init --recursive
sudo ./test/distrib/cpp/run_distrib_test_cmake.sh
cd /tmp && rm -rf grpc


cd /tmp && rm -rf minisketch
git clone https://github.com/sipa/minisketch.git
cd minisketch
./autogen.sh && ./configure --prefix=${install_dir}
make -j && make install
cd /tmp && rm -rf minisketch

cd /tmp && rm -rf CLI11
git clone https://github.com/CLIUtils/CLI11.git
git submodule update --init
cd CLI11 && mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=${install_dir}
make -j && make install
cd /tmp && rm -rf CLI11

echo "DONE!"



