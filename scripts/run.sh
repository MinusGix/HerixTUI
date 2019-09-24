#!/usr/bin/env bash

# This is meant to simply run the program with the plugins directory.
# This should not be used if you are testing locating of plugins, or want plugins from other directories.
# This does not build the program and simply runs it.

./build/program -p ./plugins/ $@