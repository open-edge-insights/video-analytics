#!/bin/bash -x

# Init the EIS env varilables
source ../../build/.env

export DEV_MODE=$DEV_MODE
export PROFILING=$PROFILING
export INFLUX_SERVER=$INFLUX_SERVER
export DEV_MODE=$DEV_MODE

# Set PythonPATH
export PYTHONPATH="../../:..:/opt/intel/openvino_2019.1.133/python/python3.6/ubuntu16/"

if [ $DEV_MODE = "false" ]
then
	# Set required environment variables for prod mode
	shared_key=`docker exec -it ia_data_agent "env" | grep SHARED_KEY | cut -d = -f 2 | tr -d '\r'`
	shared_nonce=`docker exec -it ia_data_agent "env" | grep SHARED_NONCE | cut -d = -f 2 |tr -d '\r'`
	export SHARED_KEY="$shared_key"
	export SHARED_NONCE="$shared_nonce"
fi

