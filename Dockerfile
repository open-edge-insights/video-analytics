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
    apt-get install -y libpng-dev libcairo2-dev libpango1.0-dev libglib2.0-dev libgtk-3-dev \
    libswscale-dev libavcodec-dev libavformat-dev libgstreamer1.0-0 gstreamer1.0-plugins-base \
    libusb-1.0-0-dev libva-dev libdrm-dev cpio lsb-release
COPY l_openvino_toolkit*  ${HOME}/openvino_toolkit/
RUN cd ${HOME}/openvino_toolkit && \
    sed -i -e 's/^ACCEPT_EULA=decline/ACCEPT_EULA=accept/g' silent.cfg && \
    ./install.sh -s silent.cfg --ignore-cpu

# Configure MO and install dependencies for processor graphics
RUN mkdir neo && \
    cd neo && \
    wget https://github.com/intel/compute-runtime/releases/download/19.16.12873/intel-gmmlib_19.1.1_amd64.deb && \
    wget https://github.com/intel/compute-runtime/releases/download/19.16.12873/intel-igc-core_1.0.2-1787_amd64.deb && \
    wget https://github.com/intel/compute-runtime/releases/download/19.16.12873/intel-igc-opencl_1.0.2-1787_amd64.deb && \
    wget https://github.com/intel/compute-runtime/releases/download/19.16.12873/intel-opencl_19.16.12873_amd64.deb && \
    wget https://github.com/intel/compute-runtime/releases/download/19.16.12873/intel-ocloc_19.16.12873_amd64.deb && \
    dpkg -i *.deb

# Remove OpenVINO toolkit after installation
RUN rm -rf ${HOME}/l_openvino_toolkit*

# Myriad and HDDL requirements
RUN apt install -y libboost-all-dev libusb-1.0-0 libssl1.0.0 libudev1 libjson-c3 udev    
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
COPY --from=common /util ${PY_WORK_DIR}/util

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
