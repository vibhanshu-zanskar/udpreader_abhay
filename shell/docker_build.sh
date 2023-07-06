#! /bin/bash

set -x

SCRIPT_FULLPATH="$(cd -- "$(dirname -- "${BASH_SOURCE[0]:-$0}";)" &> /dev/null && pwd 2> /dev/null;)";
PARENT_REL_DIR=$(dirname -- $BASH_SOURCE)
PROJECT_FULL_ROOT=$(dirname -- $SCRIPT_FULLPATH)

if [ "${PARENT_REL_DIR}" != "./shell" ]
then
    echo "This script needs to be invoked from outside shell directory"
    exit 1
fi

docker run --rm -it -v ${PROJECT_FULL_ROOT}:${PROJECT_FULL_ROOT} \
        --workdir ${PROJECT_FULL_ROOT} \
        --entrypoint "/bin/bash" \
        --user "$(id -u):$(id -g)" \
        --name titan.titan \
        titan:focal_boost-1.81_gcc-11_pcap shell/build.sh "$@"
