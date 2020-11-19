Buildroot: {{BUILD_ROOT}}
Name: openperf
Version: {{VERSION}}
Release: {{PKG_VERSION}}
Summary: Spirent Network and Load Generator
License: see /usr/share/doc/openperf/copyright
Distribution: Debian
Group: utils

%define _rpmdir ../
%define _rpmfilename %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm
%define _unpackaged_files_terminate_build 0

%description
Provides an infrastructure and application test and analysis framework.
Discrete modules provide functionality with the assistance of a common
core. Each module also defines a REST API via a Swagger specification
to allow users to access and control its functionality.

https://github.com/Spirent/openperf

%files
%dir "/"
%dir "/etc/"
%dir "/etc/openperf/"
%config "/etc/openperf/config.yaml"
%dir "/usr/"
%dir "/usr/bin/"
"/usr/bin/openperf"
%dir "/usr/lib/"
"/usr/lib/libopenperf-shim.so"
