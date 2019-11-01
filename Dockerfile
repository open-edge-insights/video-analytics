# Dockerfile for VideoAnalytics
ARG EIS_VERSION
FROM ia_openvino_base:$EIS_VERSION as openvino
LABEL description="VideoAnalytics image"

WORKDIR ${PY_WORK_DIR}
ARG EIS_UID
ARG EIS_USER_NAME
RUN useradd -r -u ${EIS_UID} -G video ${EIS_USER_NAME}

# Installing dependent python modules - needed by opencv
COPY va_requirements.txt .
RUN pip3.6 install -r va_requirements.txt && \
    rm -rf va_requirements.txt

FROM ia_common:$EIS_VERSION as common

FROM openvino

WORKDIR ${GO_WORK_DIR}

COPY --from=common /usr/local/include /usr/local/include
COPY --from=common /usr/local/lib /usr/local/lib
COPY --from=common ${GO_WORK_DIR}/common/cmake ./common/cmake
COPY --from=common ${GO_WORK_DIR}/common/libs ./common/libs

# Build UDF loader lib
RUN /bin/bash -c "source /opt/intel/openvino/bin/setupvars.sh && \
     cd ./common/libs/UDFLoader && \
     rm -rf build && \
     mkdir build && \
     cd build && \
     cmake .. && \
     make install"

# Adding project depedency modules
COPY . ./VideoAnalytics/

RUN chmod +x ./VideoAnalytics/va_classifier_start.sh
RUN chmod +x ./VideoAnalytics/va_start.sh
# RUN apt-get install -y libc6-dbg gdb valgrind

RUN /bin/bash -c "source /opt/intel/openvino/bin/setupvars.sh && \
    cd ./VideoAnalytics && \
    rm -rf build && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make"

ENTRYPOINT ["VideoAnalytics/va_start.sh"]

