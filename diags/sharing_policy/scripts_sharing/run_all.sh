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
CORE_ID_2=""
PERF_EVENT=""
WINDOW=""

cpuid_vendor=$(cpuid | grep vendor)
if [[ ${cpuid_vendor^^} == *"AMD"* ]]; then
	echo "I'm AMD"
	CORE_ID_1="1"
	CORE_ID_2="7"
	PERF_EVENT="cycles,instructions,r01AA,r02AA"
    WINDOW="64"
elif [[ ${cpuid_vendor^^} == *"INTEL"* ]]; then 
	echo "I'm Intel"
	CORE_ID_1="1"
	CORE_ID_2="5"
	PERF_EVENT="cycles,instructions,r0479,r0879"
    WINDOW="32"
else
	echo "Fail to identify a specific vendor"
	exit 1
fi


##################################
#### Target benchmarks
##################################
NUM_CHUNKS=(
	"2"
	"4"
	"8"
	"16"
	"32"
	"64"
	"128"
	"256"
	"512"
	"1024"
    "2048"
)



##################################
#### START Evaluation
##################################
mkdir -p ${LOGDIR}

function do_eval {
    _cbs1=`expr ${1} \\* ${WINDOW}`
    _cbs2=`expr ${2} \\* ${WINDOW}`
	first_exe="${BIN_PATH} -s ${_cbs1} -t 3 -c ${WINDOW} -n 5"
	second_exe="${BIN_PATH} -s ${_cbs2} -t 3 -c ${WINDOW} -n 5"
	first_cid=${3}
	second_cid=${4}

    echo ${first_exe}

	echo "===== Do eval (num_chunk: ${1} / ${2}) (cid: ${3} / ${4}) ===="

	# run first uBench
	taskset -c ${first_cid} ${first_exe} &
	_first_pid="$!"
	echo "*** run first bench (pid: $_first_pid) ***"

	# run second uBench
	if [ "$second_cid" == "-1" ]; then
		_second_pid=""
	else
		taskset -c ${second_cid} ${second_exe} &
		_second_pid="$!"
	fi
	echo "*** run second bench (pid: $_second_pid) ***"
	
	# warmup (3sec)
	sleep 3
	echo "*** end warmup ***"

	# Extract stat (1st & 2nd)
	if [ "$second_cid" == "-1" ]; then
		target_pids="${_first_pid}"
		pid_fname_suffix="pid_${_first_pid}_0"
	else
		target_pids="${_first_pid},${_second_pid}"
		pid_fname_suffix="pid_${_first_pid}_${_second_pid}"
	fi
	perf stat -e ${PERF_EVENT} -p ${target_pids} --per-thread > ${LOGDIR}/numChunks_${1}_${2}_${pid_fname_suffix} 2>&1 &
	perf_pid="$!"
	echo "*** START PERF STAT COLLECTION (PID:${perf_pid}) ***"

	# Extract stats (5sec)
	sleep 5
	echo "*** END PERF STAT COLLECTION ***"

	kill -INT ${perf_pid}
	killall -9 sharing_policy
	echo "*** kill all benchs & perf tools ***"
}


# second cid == -1 (single execution test)
for first_num in "${NUM_CHUNKS[@]}"; do
	do_eval ${first_num} 0 ${CORE_ID_1} -1
done

# contention test
for first_num in "${NUM_CHUNKS[@]}"; do
	for second_num in "${NUM_CHUNKS[@]}"; do
		do_eval ${first_num} ${second_num} ${CORE_ID_1} ${CORE_ID_2}
	done
done
