# UDT 4.13 for Debian 13

UDT 4.13 from [dorkbox/UDT](https://github.com/dorkbox/UDT), built for Debian 13
as `libudt0` and `libudt-dev`. `udr` links against it. The soname is
`libudt.so.0`, and the library has three calls that Debian's 4.11 lacks:
`UDT::flush` and the epoll helpers `epoll_update_usock` and
`epoll_verify_usock`. Libraries install to `/usr/lib/<multiarch>` and headers
to `/usr/include/udt`.

```sh
make && make check       # libudt.so.0.4.13, libudt.a, and a loopback transfer
./build-deb.sh           # the .deb files, in out/deb/ (Debian 13 host)
```

The tree builds with one Makefile, for Linux. On Linux the library keeps time
with `CLOCK_MONOTONIC` in microseconds; the TSC code paths serve the other
platforms, where the timer picks its assembly from the compiler's
architecture macros. `tests/loopback.cpp` runs in every package build: a
transfer at full speed and one at a capped rate, which checks the timer. The
text below is dorkbox's README.

# Breaking the Data Transfer Bottleneck

UDT is a reliable UDP based application level data transport protocol for distributed data intensive applications
 over wide area high-speed networks. UDT uses UDP to transfer bulk data with its own reliability control and 
 congestion control mechanisms. The new protocol can transfer data at a much higher speed than TCP does. UDT
  is also a highly configurable framework that can accommodate various congestion control algorithms.  
  - Presentation: [PowerPoint](https://github.com/dorkbox/UDT/blob/master/udt-doc/udt-2009.ppt)
  - Poster: [PDF](https://github.com/dorkbox/UDT/blob/master/udt-doc/udt-sc08-poster.pdf)

### TCP 

TCP is [slow](http://barchart.github.com/barchart-udt/main/presentation-2009/img6.html).
UDT is [fast](http://barchart.github.com/barchart-udt/main/presentation-2009/img9.html).

### UDT

UDT is developed by [Yunhong Gu](http://www.linkedin.com/in/yunhong) and others at University of Illinois and Google.

UDT C++ implementation is available under [BSD license](http://udt.sourceforge.net/license.html)


### Key Features

**Fast**. UDT is designed for extremely high speed networks and it has been used to support global data transfer of terabyte sized data sets. UDT is the core technology in many commercial WAN acceleration products.

**Fair and Friendly**. Concurrent UDT flows can share the available bandwidth fairly, while UDT also leaves enough bandwidth for TCP.

**Easy to Use**. UDT resides completely at the application level. Users can simply download the software and start to use it. No kernel reconfiguration is needed. In addition, UDT's API is very similar to the traditional socket API so that existing applications can be easily modified.

**Highly Configurable**. UDT supports user defined congestion control algorithms with a simple configuration. Users may also modify UDT to suit various situations. This feature can also be used by students and researchers to investigate new control algorithms.

**Firewall Friendly**. UDT is completely based on UDP, which makes it easier to traverse the firewall. In addition, multiple UDT flows can share a single UDP port, thus a firewall can open only one UDP port for all UDT connections. UDT also supports rendezvous connection setup.


### Supported Platforms

| ARCH/OS      |  Linux  | Mac OSX | Windows |
|--------------|---------|---------|---------|
| arm-android  |   ???   |         |         |
| arm-rpi      |   ???   |         |         |
| x86/i386     |   YES   |   YES   |   YES   |
| x86-64/amd64 |   YES   |   YES   |   YES   |


### Current Implementation
 - Updates to UDT source 4.11 to fix some misc. CPU timing bugs in Linux (via the sourceforge help forum).
 - Cleaned up source for cross-compile environment in linux
 - Cleaned up preprocessor symbols and removed deprecated
 - Strips unneeded symbols, drastically reducing size
 - Static linking to mingw libraries for windows build

