#!/bin/bash -x

# Copyright (c) 2020 Intel Corporation.

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

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

