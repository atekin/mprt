#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
pushd $DIR/../clang_build
cmake -DCMAKE_BUILD_TYPE=Debug ../src
popd
