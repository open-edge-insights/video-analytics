#!/bin/bash

source /opt/intel/openvino/bin/setupvars.sh

udevadm control --reload-rules
udevadm trigger

# Uncomment these lines if you are using HDDL
#$HDDL_INSTALL_DIR/bin/hddldaemon &
#sleep 20

python3.6 VideoAnalytics/video_analytics.py --log $PY_LOG_LEVEL
