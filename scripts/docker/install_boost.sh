# Script to install extra boost libraries from source

set -euo pipefail

# Boost is already in the container as leftover from casacore install
pushd /build/boost_${BOOST_}
./bootstrap.sh --prefix=/opt/boost --with-libraries=math,date_time,system
./b2 -j${THREADS} cxxflags="-fPIC" link=static,shared install

popd
