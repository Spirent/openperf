FROM spirentorion/openperf:latest AS devimg

ARG GIT_COMMIT
ARG GIT_VERSION
ARG BUILD_NUMBER
ENV GIT_COMMIT=${GIT_COMMIT}
ENV GIT_VERSION=${GIT_VERSION}
ENV BUILD_NUMBER=${BUILD_NUMBER}

COPY . /root/project/
WORKDIR /root/project

# Use all available cores for making
RUN make -j$(nproc) -C targets/openperf

FROM debian:buster-slim AS runtime

RUN apt-get clean && \
    apt-get update && \
    apt-get install -y --no-install-recommends libnuma1 libcap2

RUN mkdir -p /opt/openperf/bin
RUN mkdir -p /opt/openperf/plugins

# Environment variables for init script
ENV EXECUTABLE=/opt/openperf/bin/openperf
ENV OP_CONFIG=/etc/openperf/config.yaml
ENV OP_MODULES_PLUGINS_PATH=/opt/openperf/plugins
ENV OP_MODULES_API_PORT=9000

COPY --from=devimg /root/project/init.sh /init
COPY --from=devimg /root/project/config.yaml ${OP_CONFIG}
COPY --from=devimg /root/project/build/openperf-linux-x86_64-testing/plugins/* ${OP_MODULES_PLUGINS_PATH}
COPY --from=devimg /root/project/build/openperf-linux-x86_64-testing/bin/openperf ${EXECUTABLE}

EXPOSE ${OP_MODULES_API_PORT}/tcp
ENTRYPOINT [ "/init" ]
