#!/bin/bash

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

# Install OpenVino
apt-get install -y libpng12-0 libcairo2-dev libpango1.0-dev libglib2.0-dev libgtk2.0-dev \
    libswscale-dev libavcodec-dev libavformat-dev libgstreamer1.0-0 gstreamer1.0-plugins-base \
    build-essential cmake libusb-1.0-0-dev libva-dev libdrm-dev cpio

apt-get install -y lsb-release

cd ./l_openvino_toolkit* && \
    sed -i -e 's/^ACCEPT_EULA=decline/ACCEPT_EULA=accept/g' silent.cfg && \
    ./install.sh -s silent.cfg --ignore-cpu

cd /opt/intel/openvino/deployment_tools/model_optimizer/install_prerequisites && \
    sed -i -e "s/sudo \(-[^- ]*\)\?//g" install_prerequisites.sh && \
    sed -i '/onnx/d' ../requirements.txt && echo "onnx==1.1.2" >> ../requirements.txt && \
    sed -i '/networkx/d' ../requirements.txt && echo "networkx==1.11" >> ../requirements.txt && \
    sed -i '/numpy/d' ../requirements.txt && echo "numpy==1.12.0" >> ../requirements.txt && \
    ./install_prerequisites.sh && \
    cd /opt/intel/openvino/install_dependencies && \
    ./install_NEO_OCL_driver.sh
