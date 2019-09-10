# Dockerfile for VideoAnalytics
ARG EIS_VERSION
FROM ia_openvino_base:$EIS_VERSION as openvino
LABEL description="VideoAnalytics image"

ARG EIS_UID
ARG EIS_USER_NAME
RUN useradd -r -u ${EIS_UID} -G video ${EIS_USER_NAME}

# Installing dependent python modules
COPY va_requirements.txt .
RUN pip3.6 install -r va_requirements.txt && \
    rm -rf va_requirements.txt

FROM ia_common:$EIS_VERSION as common

FROM openvino

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
