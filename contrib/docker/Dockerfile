FROM spirentorion/openperf:latest AS devimg

COPY "." "/root/project/"
WORKDIR /root/project

ARG GIT_COMMIT
ARG GIT_VERSION
ARG BUILD_NUMBER
ENV GIT_COMMIT=${GIT_COMMIT}
ENV GIT_VERSION=${GIT_VERSION}
ENV BUILD_NUMBER=${BUILD_NUMBER}

RUN cd targets/openperf && make

FROM debian:buster-slim as runtime

RUN apt-get clean && \
    apt-get update && \
    apt-get install -y --no-install-recommends libnuma1 libcap2

RUN mkdir -p /opt/openperf/bin
RUN mkdir -p /opt/openperf/plugins

COPY --from=devimg /root/project/build/openperf-linux-x86_64-testing/bin/openperf /opt/openperf/bin/openperf
COPY --from=devimg /root/project/build/openperf-linux-x86_64-testing/plugins/* /opt/openperf/plugins
COPY --from=devimg /root/project/config.yaml /etc/openperf/config.yaml

EXPOSE 9000
CMD /opt/openperf/bin/openperf -c /etc/openperf/config.yaml
