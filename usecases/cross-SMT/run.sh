#!/bin/bash

BASEDIR=$(pwd -P)

rm -r result

###
### Victim runs
###
echo
echo "***** Victim runs *****"
pushd ${BASEDIR}/Victim > /dev/null
make -s clean
make -s
./victim -k 1101 &
_vpid="$!"
popd > /dev/null 

sleep 1

### 
### Spy runs
### 
echo
echo "***** Spy runs *****"
pushd ${BASEDIR}/Spy > /dev/null
make -s clean
make -s
./spy
_apid="$!"
popd > /dev/null 

killall -9 spy
killall -9 victim

./objdump_code_block.sh
