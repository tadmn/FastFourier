# FastFourier

A simple FFT wrapper for all the major desktop and mobile OSes. This library will always wrap the fastest (& most free)
FFT library for your platform.

This is a fork of https://github.com/hogliux/FastFourier. The original is built as a shared library (dll), whereas
this fork is built as a static library and is thus simplified somewhat (and doesn't require installation of the shared
lib alongside your binaries).

This fork supports macOS, Windows x86_64, & Linux x86_64.

## Setup

### macOS
Nothing is required. The vDSP library (via the Accelerate framework) is already installed on all macOS systems.

### Windows x86_64
You will need to download the library files for the Intel Performance Primitives (IPP). Note by running the commands
below, you are accepting Intel's EULA for the IPP library.
```
curl --output oneapi.exe https://registrationcenter-download.intel.com/akdlm/IRC_NAS/2e89fab4-e1c7-4f14-a1ef-6cddba8c5fa7/intel-ipp-2022.0.0.796_offline.exe
./oneapi.exe -s -x -f oneapi
./oneapi/bootstrapper.exe -s -c --action install --components=intel.oneapi.win.ipp.devel --eula=accept -p=NEED_VS2022_INTEGRATION=1 --log-dir=.
```

The files should get installed to `/Program Files (x86)/Intel/oneAPI/ipp`.

### Linux x86_64
You will need to download the library files for the Intel Performance Primitives (IPP). The instructions below are
tested on Ubuntu 24.04. Note by running the commands below, you are accepting Intel's EULA for the IPP library.
```
wget -O- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB | gpg --dearmor | sudo tee /usr/share/keyrings/oneapi-archive-keyring.gpg > /dev/null
echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] https://apt.repos.intel.com/oneapi all main" | sudo tee /etc/apt/sources.list.d/oneAPI.list
sudo apt update
sudo apt install intel-oneapi-ipp-devel
```

The files should get installed to `/opt/intel/oneapi/ipp`.
