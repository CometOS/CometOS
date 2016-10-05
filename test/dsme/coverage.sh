#!/bin/bash

echo "====== STEP 1 ======"
# Generate initial coverage information
lcov -b . -c -i -d ./build -o .coverage.wtest.base
 
# Run all tests:
./test
 
# Generate coverage based on executed tests
lcov -b . -c -d ./build -o .coverage.wtest.run
 
echo "====== STEP 2 ======"
# Merge coverage tracefiles
lcov -a .coverage.wtest.base -a .coverage.wtest.run  -o .coverage.total

# Filtering, extracting project files

echo "====== STEP 3 ======"
cd ..
parentname="$(pwd)"
echo $parentname
cd test
lcov -e .coverage.total "`echo $parentname`/*" -o .coverage.total.filtered
 
echo "====== STEP 4 ======"
# Filtering, removing test-files
lcov -r .coverage.total.filtered "`pwd`/*_unittest.h" -o .coverage.total.filtered
lcov -r .coverage.total.filtered "`pwd`/build/test.cc" -o .coverage.total.filtered
lcov -r .coverage.total.filtered "`pwd`/build/gtest-all.cc" -o .coverage.total.filtered
lcov -r .coverage.total.filtered "`pwd`/gtest/*" -o .coverage.total.filtered
 
# Extra:  Replace /build/ with original path to unify directories
sed 's/\/test\/build\//\//g' .coverage.total.filtered > .coverage.total
 
# Extra: Clear up previous data, create html folder
if [[ -d ./html/ ]] ; then
    rm -rf ./html/*
else
    mkdir html
fi
 
echo "====== STEP 5 ======"
# Generate webpage
genhtml -o ./html/ .coverage.total
 
# Cleanup
rm .coverage.*
rm -rf ./build
rm test
