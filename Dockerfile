# Dockerfile for VideoAnalytics
ARG EIS_VERSION
FROM ia_pybase:$EIS_VERSION as pybase
LABEL description="VideoAnalytics image"

ARG EIS_UID
ARG EIS_USER_NAME
RUN useradd -r -u ${EIS_UID} -G video ${EIS_USER_NAME}

ENV HOME /EIS

# OpenVINO install
RUN apt-get update && \
    apt-get install -y libpng12-dev libcairo2-dev libpango1.0-dev libglib2.0-dev libgtk2.0-dev \
    libswscale-dev libavcodec-dev libavformat-dev libgstreamer1.0-0 gstreamer1.0-plugins-base \
    libusb-1.0-0-dev libva-dev libdrm-dev cpio lsb-release
COPY l_openvino_toolkit*  ${HOME}/openvino_toolkit/
RUN cd ${HOME}/openvino_toolkit && \
    sed -i -e 's/^ACCEPT_EULA=decline/ACCEPT_EULA=accept/g' silent.cfg && \
    ./install.sh -s silent.cfg --ignore-cpu

# Configure MO and install dependencies for processor graphics
RUN bash -c 'source /opt/intel/openvino/bin/setupvars.sh && \
    cd /opt/intel/openvino/deployment_tools/model_optimizer/install_prerequisites && \
    sed -i -e "s/sudo \(-[^- ]*\)\?//g" install_prerequisites.sh && \
    sed -i '/onnx/d' ../requirements.txt && echo "onnx==1.1.2" >> ../requirements.txt && \
    sed -i '/networkx/d' ../requirements.txt && echo "networkx==1.11" >> ../requirements.txt && \
    sed -i '/numpy/d' ../requirements.txt && echo "numpy==1.12.0" >> ../requirements.txt && \
    ./install_prerequisites.sh && \
    cd /opt/intel/openvino/install_dependencies && \
    ./install_NEO_OCL_driver.sh'

# Remove OpenVINO toolkit after installation
RUN rm -rf ${HOME}/l_openvino_toolkit*

# Myriad and HDDL requirements
RUN apt install -y libusb-1.0-0 libboost-program-options1.58.0 libboost-thread1.58.0 \
    libboost-filesystem1.58.0 libssl1.0.0 libudev1 libjson-c2 udev 
COPY 97-myriad-usbboot.rules .    
RUN echo "source /opt/intel/openvino/bin/setupvars.sh" >> ~/.bashrc && \
    cp /EIS/97-myriad-usbboot.rules /etc/udev/rules.d/ && \
    echo "udevadm control --reload-rules" >> ~/.bashrc && \
    echo "udevadm trigger" >> ~/.bashrc 

# Installing dependent python modules
COPY va_requirements.txt .
RUN pip3.6 install -r va_requirements.txt && \
    rm -rf va_requirements.txt

FROM ia_common:$EIS_VERSION as common

FROM pybase

COPY --from=common /libs ${PY_WORK_DIR}/libs
COPY --from=common /Util ${PY_WORK_DIR}/Util

RUN cd ./libs/EISMessageBus && \
    rm -rf build deps && \
    mkdir build && \
    cd build && \
    cmake -DWITH_PYTHON=ON .. && \
    make && \
    make install

ENV PYTHONPATH ${PYTHONPATH}:.

# Adding project depedency modules
COPY . ./VideoAnalytics/

RUN chmod +x ./VideoAnalytics/va_classifier_start.sh

ENTRYPOINT ["./VideoAnalytics/va_classifier_start.sh"]
HEALTHCHECK NONE
