#!/bin/bash

SCRIPT_FULLPATH="$(cd -- "$(dirname -- "${BASH_SOURCE[0]:-$0}";)" &> /dev/null && pwd 2> /dev/null;)";
PARENT_REL_DIR=$(dirname -- $BASH_SOURCE)
PROJECT_FULL_ROOT=$(dirname -- $(dirname -- $SCRIPT_FULLPATH))

if [ "${PARENT_REL_DIR}" != "./shell" ]
then
    echo "This script needs to be invoked from outside shell directory"
    exit 1
fi

        #--user "$(id -u):$(id -g)" \
docker run --rm -it -v ${PROJECT_FULL_ROOT}:/projects \
        -v $HOME/titan_data:/titan_data \
        --workdir /projects/titan \
        --entrypoint "/bin/bash" \
        --name titan.udpreader \
        titan:focal_boost-1.81_gcc-11_pcap
