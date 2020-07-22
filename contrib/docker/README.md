# Build and run OpenPerf contribution Docker image

OpenPerf contribution Docker image is a lightweight image based on debian:buster-slim distribution and contains only OpenPerf executable with minimum dependencies. It uses multi-stage build to reduce the size of the image.

## Build the Docker image
```
make image
```

## Run the tests
```
make image_test_aat
```

## Make a tar archive with OpenPerf image
```
make image_pack
```

## Run OpenPerf as Docker image
```
docker run -it -p 9000:9000 --privileged openperf-contrib:latest
```
Configuration file is located at `/etc/openperf/config.yaml`. It can be replaced using `-v` option of `docker run` command or analogues.

## Clean Docker from OpenPerf and intermediate images
```
make image_clean
```
