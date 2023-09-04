#!/bin/bash

set -euxo pipefail

if [ $# -lt 1 ]; then
  echo 1>&2 "$0: requires one arg that is EXTERNAL/INTERNAL/LOCAL"
  exit 2
fi

RUN_ENV=$1
echo "Running in mode $RUN_ENV"

if [ "$RUN_ENV" == "EXTERNAL" ];
then
    BUILD_TYPE=Release
elif  [ "$RUN_ENV" == "INTERNAL" ];
then
    BUILD_TYPE=RelWithDebInfo
elif [ "$RUN_ENV" == "LOCAL" ];
then
    BUILD_TYPE=Debug
else
    echo "Run Env: $RUN_ENV not identified "
    exit 2
fi

BUILD_DIR=build/build_${BUILD_TYPE}

if [ -d ${BUILD_DIR} ]
then
    rm -rf ${BUILD_DIR}
fi

mkdir -p ${BUILD_DIR}
pushd $BUILD_DIR
cmake -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_BUILD_TYPE=$BUILD_TYPE ../..
make
popd

cp $BUILD_DIR/nsereader .

COPY_TO_ESTEE=false
if [ $# -gt 1 ]; then
    COPY_TO_ESTEE=$2
fi

if [ "${COPY_TO_ESTEE}" == true ]; then
    scp $BUILD_DIR/nsereader abhay@65.109.113.208:~/udpreader/
    ssh -t abhay@65.109.113.208 "scp -P 64131 ~/udpreader/nsereader estee@42.104.66.75:~/abhay/"
fi
