# Memory Address Tracer (MAT)
[comment]: # (Comment)
[paper]: <https://dl.acm.org/doi/10.1145/3399666.3399896>

Modification of Linux's perf subsystem (+ kernel module) for collecting memory traces with a low runtime overhead on Intel processors using PEBS.
See the [paper][paper] for examples how to use memory traces for analyzing database systems:

Stefan Noll, Jens Teubner, Norman May, Alexander Böhm: *Analyzing Memory Accesses with Modern Processors*, Proceedings of the 16th International Workshop on Data Management on New Hardware, 2020.

## Installation

### Overview
* build, patch, and install modified verson of Linux kernel and perf (-> patch/)
* build and install kernel module (-> module/)
* profile your application with perf and custom scripts (-> scripts/)

### Install Dependencies
```sh
### Ubuntu (may require additional dependecies)
sudo apt-get install git build-essential libncurses-dev bison flex libssl-dev libelf-dev
### if one of the following installation steps fails,
### make sure to install the necessary dependencies: error message + search engine
```
### Get Linux Kernel
```sh
### get Linux kernel version 5.1.10 (might also work for other versions)
git clone --single-branch --branch linux-5.1.y git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git linux-5.1.10-stable
cd linux-5.1.10-stable
git checkout linux-5.1.y
git checkout -b linux-5.1.10 v5.1.10
### make sure that kernel version is 5.1.10
make kernelversion
### apply kernel path (-> patch/)
git apply <patch>
```
### Configure Kernel (Example)
```sh
### configure kernel based on current kernel configuration
### copy current kernel config
cp -v /boot/config-$(uname -r) .config
### set new/updated config options
### if unsure, choose default option
make oldconfig
### or: always choose default option
yes "" | make oldconfig
```
### Build and Install Kernel
```sh
make -j$(nproc)
sudo make -j$(nproc) modules_install
sudo make -j$(nproc) install
### choose new kernel version on next boot
### choose right entry of boot manager during boot 
### or use, e.g., grub2-once to set boot entry for next boot before rebooting
### get <number> of boot entry
sudo grub2-once --list
### set boot entry of next boot
sudo grub2-once <number>
### reboot into new kernel
sudo reboot
```
### Build and Install perf
We build and install perf from sources to assure perf tool is compatible with newly installed kernel.
```sh
### get dependecies (Ubuntu)
sudo apt install libdwarf-dev libdw-dev binutils-dev libgtk2.0-dev libaudit-dev libelf-dev libnuma-dev libperl-dev libslang2-dev libunwind-dev zlib1g-dev liblzma-dev libaio-dev libssl-dev asciidoc xmlto
### build perf from sources of Linux kernel
### choose an installation directory by setting DESTDIR
cd tools/perf
make DESTDIR=/usr
sudo make DESTDIR=/usr install
### add paths (if not automatically done by installing to, e.g., "/usr")
export PATH="$PATH:<path>/bin"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:<path>/lib"
export PERF_EXEC_PATH="<path>/libexec"
export MANPATH="<share>/share/man"
source <path>/etc/bash_completion.d/perf
### make sure called perf version corresponds to installed kernel version
perf --version
```
### Build and Install Kernel Modulei
We assume kernel sources are located it "/lib/modules/$(uname -r)/build/" (see Makefle).
```sh
cd module/
make
make load
### check module was loaded proberly: several files should become avaiable via sysfs
make check
```

## Collect Memory Traces with perf (Example)
```sh
### configure kernel module to enable tracing memory addresses with perf
### this disables normal profiling with perf
### using a buffer size of 1G per core
./scripts/module.sh reset
./scripts/module.sh --buffer-size 1G set
### check buffer of every core (is empty)
./scripts/module.sh showbuffers
### profile <command> to trace memory addresses of memory loads
sudo perf record --data --event=mem_uops_retired.all_loads:pp --count=1000 --verbose -- <command>
### check buffer of every core (may contain address data)
./scripts/module.sh showbuffers
### store collected memory addresses on disk
./scripts/module.sh write
### disable tracing memory addresses with kernel module and remove per-core buffers
### this enables normal profiling with perf again
./scripts/module.sh reset
### convert binary address data to hexadecimal ascii representation for postprocessing
./scripts/binaryToHex.py <binary data> > <ascii output>
### do postprocessing and visualize...
```

Don't hesitate to create an issue on github in case of any problems.

License
----
GPL v2
