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
SMALLER_CHUNK=""

cpuid_vendor=$(cpuid | grep vendor)
if [[ ${cpuid_vendor^^} == *"AMD"* ]]; then
	echo "I'm AMD"
	CORE_ID_1="1"
	PERF_EVENT="cycles,instructions,r01AA,r02AA"
    SMALLER_CHUNK="32"
elif [[ ${cpuid_vendor^^} == *"INTEL"* ]]; then 
	echo "I'm Intel"
	CORE_ID_1="1"
	PERF_EVENT="cycles,instructions,r0479,r0879"
    SMALLER_CHUNK="32"
else
	echo "Fail to identify a specific vendor"
	exit 1
fi


##################################
#### Target benchmarks
##################################
CODE_BLOCK_SIZE=(
#	"64"
#	"128"
#	"256"
#	"512"
	"1024"
    "2048"
	"3072"
	"4096"
	"6144"
	"8192"
	"16384"
	"32768"
	"65536"
)


##################################
#### START Evaluation
##################################
mkdir -p ${LOGDIR}

function do_eval {
	first_exe="${BIN_PATH} -s ${1} -t 1 -c ${2} -n ${3}"
	first_cid=${4}

	echo "===== Do eval (cbsz: ${1}) (chunk: ${2}) (num nop: ${3}) (cid: ${4}) ===="

	# run first uBench
	taskset -c ${first_cid} ${first_exe} &
	_first_pid="$!"
	echo "*** run first bench (pid: $_first_pid) ***"

	# warmup (5sec)
	sleep 5
	echo "*** end warmup ***"

    target_pids="${_first_pid}"
    pid_fname_suffix="pid_${_first_pid}_0"
	
    perf stat -e ${PERF_EVENT} -p ${target_pids} --per-thread > ${LOGDIR}/numuop_${1}_${3}_${pid_fname_suffix} 2>&1 &
	perf_pid="$!"
	echo "*** START PERF STAT COLLECTION (PID:${perf_pid}) ***"

	# Extract stats (5sec)
	sleep 5
	echo "*** END PERF STAT COLLECTION ***"

	kill -INT ${perf_pid}
	killall -9 sharing_policy
	echo "*** kill all benchs & perf tools ***"
}

# num uop: 4
for first_cbsz in "${CODE_BLOCK_SIZE[@]}"; do
	do_eval ${first_cbsz} ${SMALLER_CHUNK} 4 ${CORE_ID_1}
done

# num uop: 5
for first_cbsz in "${CODE_BLOCK_SIZE[@]}"; do
	do_eval ${first_cbsz} ${SMALLER_CHUNK} 5 ${CORE_ID_1}
done

# num uop: 6
for first_cbsz in "${CODE_BLOCK_SIZE[@]}"; do
	do_eval ${first_cbsz} ${SMALLER_CHUNK} 6 ${CORE_ID_1}
done

# num uop: 7
for first_cbsz in "${CODE_BLOCK_SIZE[@]}"; do
	do_eval ${first_cbsz} ${SMALLER_CHUNK} 7 ${CORE_ID_1}
done

# num uop: 8
for first_cbsz in "${CODE_BLOCK_SIZE[@]}"; do
	do_eval ${first_cbsz} ${SMALLER_CHUNK} 8 ${CORE_ID_1}
done

# num uop: 9
for first_cbsz in "${CODE_BLOCK_SIZE[@]}"; do
	do_eval ${first_cbsz} ${SMALLER_CHUNK} 9 ${CORE_ID_1}
done

# num uop: 10
for first_cbsz in "${CODE_BLOCK_SIZE[@]}"; do
	do_eval ${first_cbsz} ${SMALLER_CHUNK} 10 ${CORE_ID_1}
done
