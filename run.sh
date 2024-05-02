#!/bin/sh
export WORKSPACE_ROOT=~/cpp_workspace
export BOOST_VERSION=1.85.0
export BOOST_ROOT=$WORKSPACE_ROOT/boost_$BOOST_VERSION
cmake -H. -B_build
cmake --build _build --target repro
./_build/repro
