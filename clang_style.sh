#!/bin/bash

find ./src -iname *.h -o -iname *.cc | xargs clang-format -i
