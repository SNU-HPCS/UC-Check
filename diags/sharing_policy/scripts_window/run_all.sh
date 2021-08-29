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

BIN_PATH=${BASEDIR}/../build/sharing_policy


##################################
#### Vendor-specific params.
##################################
CORE_ID_1=""
PERF_EVENT=""

cpuid_vendor=$(cpuid | grep vendor)
if [[ ${cpuid_vendor^^} == *"AMD"* ]]; then
	echo "I'm AMD"
	CORE_ID_1="1"
	PERF_EVENT="cycles,instructions,r01AA,r02AA"
elif [[ ${cpuid_vendor^^} == *"INTEL"* ]]; then 
	echo "I'm Intel"
	CORE_ID_1="1"
	PERF_EVENT="cycles,instructions,r0479,r0879"
else
	echo "Fail to identify a specific vendor"
	exit 1
fi


##################################
#### Target benchmarks
##################################
NUM_CHUNKS=(
	"32"
	"64"
	"128"
	"256"
	"512"
	"1024"
    "2048"
	"4096"
	"8192"
)


##################################
#### START Evaluation
##################################
mkdir -p ${LOGDIR}

function do_eval {
    _cbs=`expr ${1} \\* ${2}`
	first_exe="${BIN_PATH} -s ${_cbs} -t 1 -c ${2}"
	echo ${first_exe}
	first_cid=${3}

	echo "===== Do eval (num_chunk: ${1}) (chunk: ${2}) (cid: ${3}) ===="

	# run first uBench
	taskset -c ${first_cid} ${first_exe} &
	_first_pid="$!"
	echo "*** run first bench (pid: $_first_pid) ***"

	# warmup (5sec)
	sleep 5
	echo "*** end warmup ***"

    target_pids="${_first_pid}"
    pid_fname_suffix="pid_${_first_pid}_0"
	
    perf stat -e ${PERF_EVENT} -p ${target_pids} --per-thread > ${LOGDIR}/window_${1}_${2}_${pid_fname_suffix} 2>&1 &
	perf_pid="$!"
	echo "*** START PERF STAT COLLECTION (PID:${perf_pid}) ***"

	# Extract stats (5sec)
	sleep 5
	echo "*** END PERF STAT COLLECTION ***"

	kill -INT ${perf_pid}
	killall -9 sharing_policy
	echo "*** kill all benchs & perf tools ***"
}

for first_cbsz in "${NUM_CHUNKS[@]}"; do
	do_eval ${first_cbsz} 128 ${CORE_ID_1}
done

for first_cbsz in "${NUM_CHUNKS[@]}"; do
	do_eval ${first_cbsz} 64 ${CORE_ID_1}
done

for first_cbsz in "${NUM_CHUNKS[@]}"; do
	do_eval ${first_cbsz} 32 ${CORE_ID_1}
done

for first_cbsz in "${NUM_CHUNKS[@]}"; do
	do_eval ${first_cbsz} 16 ${CORE_ID_1}
done
