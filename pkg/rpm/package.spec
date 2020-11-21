Name: openperf
Version: {{VERSION}}
Release: {{PKG_VERSION}}
Summary: Spirent Network and Load Generator
License: Apache License 2.0
Vendor: Spirent Communications
Group: utils

%define _rpmdir {{OUTPUT_DIR}}
%define _rpmfilename %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm
%define _unpackaged_files_terminate_build 0

%description
Provides an infrastructure and application test and analysis framework.
Discrete modules provide functionality with the assistance of a common
core. Each module also defines a REST API via a Swagger specification
to allow users to access and control its functionality.

https://github.com/Spirent/openperf

%files
%defattr(-,root,root)
%config %attr(644, root, root) "/etc/openperf/config.yaml"
%attr(755, root, root) "/usr/bin/openperf"
%attr(755, root, root) "/usr/lib/libopenperf-shim.so"
