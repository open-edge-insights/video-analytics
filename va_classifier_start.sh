#!/bin/bash

source /opt/intel/openvino/bin/setupvars.sh

udevadm control --reload-rules
udevadm trigger

./VideoAnalytics/build/video-analytics
