#!/usr/bin/env bash

set -e

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
cp -r include "${install_dir}"
cd /tmp && rm -rf stats
echo "DONE!"
