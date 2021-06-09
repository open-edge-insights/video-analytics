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

# Dockerfile for VideoAnalytics

ARG EII_VERSION
ARG DOCKER_REGISTRY
ARG OPENVINO_IMAGE_VERSION
FROM ${DOCKER_REGISTRY}ia_video_common:$EII_VERSION as video_common
FROM ${DOCKER_REGISTRY}ia_openvino_base:$EII_VERSION as openvino_base
FROM ${DOCKER_REGISTRY}ia_eiibase:$EII_VERSION as builder
LABEL description="VideoAnalytics image"

# Installing dependent python modules - needed by opencv
WORKDIR /app

ARG CMAKE_INSTALL_PREFIX
COPY --from=video_common ${CMAKE_INSTALL_PREFIX}/include ${CMAKE_INSTALL_PREFIX}/include
COPY --from=video_common ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib
COPY --from=video_common ${CMAKE_INSTALL_PREFIX}/bin ${CMAKE_INSTALL_PREFIX}/bin
COPY --from=video_common /root/.local/bin/cythonize /root/.local/bin/cythonize
COPY --from=video_common /root/.local/lib/python3.8/site-packages/ /root/.local/lib/python3.8/site-packages
COPY --from=video_common /eii/common/cmake ./common/cmake
COPY --from=video_common /eii/common/libs ./common/libs
COPY --from=video_common /eii/common/util ./common/util
COPY --from=openvino_base /opt/intel /opt/intel

# Copy src code
COPY . ./VideoAnalytics
ENV LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:${CMAKE_INSTALL_PREFIX}/lib:${CMAKE_INSTALL_PREFIX}/lib/udfs
RUN chmod +x ./VideoAnalytics/va_classifier_start.sh
RUN /bin/bash -c "source /opt/intel/openvino/bin/setupvars.sh && \
                  cd VideoAnalytics && \
                  rm -rf build && \
                  mkdir build && \
                  cd build && \
                  cmake -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_PREFIX}/include -DCMAKE_INSTALL_PREFIX=$CMAKE_INSTALL_PREFIX -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} .. && \
                  make"

FROM openvino/ubuntu20_data_runtime:$OPENVINO_IMAGE_VERSION as runtime
USER root
ARG EII_UID
ARG EII_USER_NAME
RUN useradd -r -u ${EII_UID} -G video ${EII_USER_NAME}

WORKDIR /app
ARG CMAKE_INSTALL_PREFIX
COPY --from=builder ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib
COPY --from=builder /app/common/util/__init__.py common/util/
COPY --from=builder /app/common/util/*.py common/util/
COPY --from=builder /app/VideoAnalytics/build/video-analytics ./VideoAnalytics/build/
COPY --from=builder /app/VideoAnalytics/schema.json ./VideoAnalytics/
COPY --from=builder /app/VideoAnalytics/*.sh ./VideoAnalytics/
COPY --from=builder /root/.local/lib/python3.8/site-packages .local/lib/python3.8/site-packages
COPY --from=video_common /root/.local/lib .local/lib

COPY --from=video_common /eii/common/video/udfs/python ./common/video/udfs/python
ENV PYTHONPATH ${PYTHONPATH}:/app/common/video/udfs/python:/app/common/:/app
ENV LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:${CMAKE_INSTALL_PREFIX}/lib:${CMAKE_INSTALL_PREFIX}/lib/udfs
RUN chown ${EII_USER_NAME}:${EII_USER_NAME} /app /var/tmp
RUN usermod -a -G users ${EII_USER_NAME}
USER ${EII_USER_NAME}
HEALTHCHECK NONE
ENTRYPOINT ["./VideoAnalytics/va_classifier_start.sh"]
