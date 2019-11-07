#!/bin/bash
arg=`echo $1`
source /opt/intel/openvino/bin/setupvars.sh

./VideoAnalytics/build/video-analytics $arg
