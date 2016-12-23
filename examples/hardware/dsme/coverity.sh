#!/bin/bash
rm -r build
rm -r bin

cov-build --dir cov-int cob asserting='long'
tar czvf coverity-analysis.tgz cov-int > /dev/null 2>&1
rm -r cov-int

curl --form token=rf90xtADZXCxw_YBaFNAkQ \
  --form email=maxkoestler@yahoo.de \
  --form file=@coverity-analysis.tgz \
  https://scan.coverity.com/builds?project=CometOS%2FCometOS

rm coverity-analysis.tgz
