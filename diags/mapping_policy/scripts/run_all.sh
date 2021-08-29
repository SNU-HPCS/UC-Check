#!/bin/bash
BASEDIR=$(pwd -P)/$(dirname $0)

if [ "$EUID" -ne 0 ]; then
	echo "Please run as root"
	exit 2
fi

if [ "$#" -eq "1" ]; then
	TESTCASE=$1
else
	echo "Usage $0 [testcase]"
	exit 1
fi

DATE=$(date "+%y%m%d%H%M%S")
TESTNAME=${TESTCASE}_${DATE}
LOGDIR=${BASEDIR}/../expdata/${TESTNAME}

##################################
#### Build 
##################################
mkdir -p ${BASEDIR}/../build
rm -rf ${BASEDIR}/../build/*
pushd ${BASEDIR}/../build
cmake ..
make -j
popd

BIN_PATH=${BASEDIR}/../build/mapping_policy


##################################
#### Vendor-specific params.
##################################
CORE_ID_1=""
CORE_ID_2=""
PERF_EVENT=""

cpuid_vendor=$(cpuid | grep vendor)
if [[ ${cpuid_vendor^^} == *"AMD"* ]]; then
	echo "I'm AMD"
	CORE_ID_1="1"
	CORE_ID_2="7"
	PERF_EVENT="cycles,instructions,r01AA,r02AA"
elif [[ ${cpuid_vendor^^} == *"INTEL"* ]]; then 
	echo "I'm Intel"
	CORE_ID_1="1"
	CORE_ID_2="5"
	PERF_EVENT="cycles,instructions,r0479,r0879"
else
	echo "Fail to identify a specific vendor"
	exit 1
fi


##################################
#### Target benchmarks
##################################
MAX_SET_SIZE=130
MAX_WAY_SIZE=20
CODE_CHUNK_SIZES=(
	"16"
	"32"
	"64"
	"128"
)


##################################
#### START Evaluation
##################################
mkdir -p ${LOGDIR}

function do_eval {
  code_chunk_size=${1}
  set_size=${2}
  way_size=${3}
  first_cid=${4}
	first_exe="${BIN_PATH} -t 1 -s ${set_size} -w ${way_size} -c ${code_chunk_size}"

	echo "===== Do eval (chksz: ${code_chunk_size}) (set/way: ${set_size} / ${way_size}) ===="

	# run first uBench
	taskset -c ${first_cid} ${first_exe} &
	_first_pid="$!"
	echo "*** run first bench (pid: $_first_pid) ***"

	# warmup (1sec)
	sleep 1
	echo "*** end warmup ***"

	# Extract stat (1st & 2nd)
	target_pids="${_first_pid}"
	logfname="chksz-${code_chunk_size}_set-${set_size}_way-${way_size}_pid-${_first_pid}"
	perf stat -e ${PERF_EVENT} -p ${target_pids} --per-thread > ${LOGDIR}/${logfname} 2>&1 &
	perf_pid="$!"
	echo "*** START PERF STAT COLLECTION (PID:${perf_pid}) ***"

	# Extract stats (2sec)
	sleep 2
	echo "*** END PERF STAT COLLECTION ***"

	kill -INT ${perf_pid}
	killall -9 mapping_policy
	echo "*** kill all benchs & perf tools ***"
}

for code_chunk_size in "${CODE_CHUNK_SIZES[@]}"; do
  for set_size in `seq 1 ${MAX_SET_SIZE}`; do
    for way_size in `seq 1 ${MAX_WAY_SIZE}`; do
      do_eval ${code_chunk_size} ${set_size} ${way_size} ${CORE_ID_1}
    done
  done
done
