
# Building OpenPerf


First, checkout the git folder

	git clone https://github.com/Spirent/openperf

Then download the sub-modules dependencies

	make -C openperf deps

Then, build the docker image to get all tools installed (needed for C++17 compilation):

	docker build -t openperf-builder openperf

Then build the OpenPerf code from within the docker container:

	docker run -it --privileged -v `pwd`/openperf:/project openperf-builder:latest /bin/bash -c "make -C /project all"

If all is fine, you should see something like

	sudo -n /sbin/setcap CAP_IPC_LOCK,CAP_NET_ADMIN,CAP_NET_RAW=ep /project/build/openperf-linux-x86_64-testing/bin/openperf
	make[1]: Leaving directory '/project/targets/openperf'
	make[1]: Entering directory '/project/targets/libopenperf-shim'
	clang++ -flto=thin -shared -o /project/build/libopenperf-shim-linux-x86_64-testing/lib/libopenperf-shim.so -fuse-ld=lld -static-libstdc++ -static-libgcc -shared -L/project/build/libopenperf-shim-linux-x86_64-testing/lib /project/build/libopenperf-shim-linux-x86_64-testing/obj/icp-shim.o /project/build/libopenperf-shim-linux-x86_64-testing/obj/libc_wrapper.o -lop_socket_client -lrt -ldl
	make[1]: Leaving directory '/project/targets/libopenperf-shim'
	root@8f97d154f160:/project#
	root@8f97d154f160:/project#

Finally, run the tests

	docker run -it --privileged -v `pwd`/openperf:/project openperf-builder:latest /bin/bash -c "make -C /project test"

For verbose test, you can run the following command within the docker container:

	cd /project/tests/aat;
	env/bin/mamba spec --format=documentation

You will be able to see the following logs:

	Modules,
	  list,
	    all,
	      ✓ it returns list of modules (1.3598 seconds)
	      unsupported method,
	        ✓ it returns 405
	  get,
	    known module,
	      packetio
	        ✓ it succeeds
	    non-existent module,
	      ✓ it returns 404
	    unsupported method,
	      ✓ it returns 405
	    invalid module name,
	      ✓ it returns 400

# Build and run Openperf contribution docker image

## Build the docker image
```
make image
```

## Run the tests
```
make image_test_aat
```

## Make a tar archive with Openperf image
```
make image_pack
```

## Run Openperf as docker image
```
docker run -it -p 9000:9000 --privileged openperf-contrib:latest
```
Configuration file is located at `/etc/openperf/config.yaml`. It can be replaced using `-v` option of `docker run` command or analogues.

## Clean docker from Openperf and intermediate images
```
make image_clean
```