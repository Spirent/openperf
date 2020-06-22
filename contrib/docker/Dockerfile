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

FROM debian:buster as runtime

RUN apt-get clean && \
    apt-get update && \
    apt-get install -y --no-install-recommends libnuma1

COPY --from=devimg /root/project/build/openperf-linux-x86_64-testing/bin/openperf /usr/local/bin/openperf
COPY --from=devimg /root/project/contrib/docker/config.yaml /etc/openperf/config.yaml

EXPOSE 9000
CMD /usr/local/bin/openperf -c /etc/openperf/config.yaml
