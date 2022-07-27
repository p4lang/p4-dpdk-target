## [v22.07] - 2022-07-27

### Added

* Linux Networking support (L2 Forwarding, VXLAN, Routing, Connection Tracking)
* TDI Integration and TDI CLI Python support.
* PAL API support to retrieve fixed table functionality such as port, device etc.
* Direct Match Table support [Match kind supported - Exact, Ternary, LPM].
* Indirect Match Table support - Action Selector, Action Profile.
* Indirect Counter support.
* Port and Vport support.
* Auto learn support.
* PNA meta data direction support.
* Table default action with arguments support.
* Multiple Pipeline support.
* Hotplug support for vhost-user ports.
* Enable DPDK backend cli to get port/mac stats.
* GTEST based Unit test framework.

### Fixed

* Table name is different in ContextJSON and SPEC artifacts/file.
* Display of custom MTU value for TAP port set by gnmi-cli. 
* Action profile/selector: table entry addition is failing for action profile or selector tests.
* Action selector: deleting non-existing group (the first one) did not generate error.
* Indirect Counter: Flow / Indirect counters does not account packet counters.
* Indirect Counter: Reset-counter not working for flow counters.

### Limitation

* Counter: No support for Counter cell display when index is not set.
* Counter: Cumulative count of all counters not supported (index is 0).
* Configure invalid port in rule and sending traffic causes application to crash.
* Add/Remove Hot plug interface on VM1/VM2 does not resume ping traffic on any VM in the same host in LNT Feature.
* Need to set PYTHONPATH and PYTHONHOME for tdi_python to work and LD_LIBRARY_PATH to the custom library path.
* Logging: Following error logs of type "BF_BFRT ERROR" and "BF_BFRT FATAL" generated when lanunching the OVS but 
  no functional impact.
