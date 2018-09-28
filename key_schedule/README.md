Copyright (C) 2017 Netronome Systems, Inc.  All rights reserved.

-------------------------------------------------------------------------------
*** Requirements
-------------------------------------------------------------------------------
*** Supported Operating Systems ***
The following operating systems are supported:
Ubuntu 14.04 LTS - tested with various releases of kernel 3.13 to kernel 4.2
Ubuntu 16.04 LTS - tested with various releases of kernel 4.4 to kernel 4.10
CentOS 6 - tested with various releases of kernel 2.6.32
CentOS 7 - tested with various releases of kernel 3.10.0

Although not tested SDK6 RTE should work with other versions of the operating 
systems and kernels up to kernel version 4.10.

Please read the section lower down about the required patching for kernels < 4.5
due to the PCIe quirk/ERR47.

*** SR-IOV for PCIe Virtual Functions ***
If packet transfer between the host and SmartNIC is required it will need to be
done with PCIe Virtual Functions (VF) . This requires a PCIe Physical Function 
that support Alternative ID-routing Interpretation (ARI) capability and 
implement Single Rout I/O Virtualization (SR-IOV).Since SR-IOV is optional part 
of PCIe specification and seen as a "server-only" feature is is typically only 
supported on server motherboard chipsets. Please refer to your intended test 
setup's hardware documentation on whether it is supported.  

To enable SR-IOV and ARI functionality it needs to be enabled in the system BIOS
and the operating system. To enable in the BIOS typically requires 
Virtualization Technology (Intel VT-d or AMD-Vi) and SR-IOV to be enabled 
globally; some BIOS might also require SR-IOV to be enabled on the specific PCIe
port being used (please refer to your hardware manufacturer's documentation for 
the specifics).  In the OS virtualization needs be enabled in the kernel startup
settings using intel_iommu=on or amd_iommu=on.

-------------------------------------------------------------------------------
*** Dependencies
-------------------------------------------------------------------------------
The following packages should be installed on Ubuntu before installing SDK6 
RTE and BSP (default install). The command below will install these packages 
(must be run as root or with sudo):

apt-get install realpath libftdi1 libjansson4 build-essential \
 linux-headers-`uname -r` dkms

If doing an RTE only install for use with the simulator only the following 
subset of the above packages are needed:

apt-get install realpath libjansson4 build-essential

For the CentOS/RHEL installation, the epel repo must be enabled to install all
the required dependencies. This can be disabled again after installation. The
following command can be used to enable this repo.

yum install epel-release or downloading the EPEL repo install rpm directly.

The packages for a full RTE and BSP installation (default install) on 
CentOS/RHEL can be installed with the following command (must be run as root or 
with sudo):

yum install libftdi jansson pciutils kernel-devel dkms

If doing an RTE only install for use with the simulator only the following 
subset of the above packages are needed:

yum install jansson

-------------------------------------------------------------------------------
*** Compatibility with other Netronome host system software products
-------------------------------------------------------------------------------
Please ensure all other Netronome host system software products (ex Agilio) are
not running and will not be started on system start before attempting to install
or run SDK6 RTE.

In the case of ns-agilio-corenic please uninstall it from the system and restart
the host before attempting to install SDK6 RTE.

-------------------------------------------------------------------------------
*** Installing SDK6 Run Time Environment and Hardware Debug server 
-------------------------------------------------------------------------------
This is the recommended installation procedure for host with Netronome ISA's.

The "install" command with the included installs script will install the 
Hardware Debug Server and SDK6 RTE, the installed BSP version will be checked 
and the user prompted to install/update with the packaged BSP version. It is 
RECOMMNEDED to update the NFP BSP tools when prompted:
  ./sdk6_rte_install.sh install

To force an update of the BSP to version packaged with install tarball:
  ./sdk6_rte_install.sh install_force_bsp  
  
To check the currently installed BSP and packaged BSP:
  ./sdk6_rte_install bsp_version_info
  
All install script commands must be run with sudo or as root.
  
NOTE:
If firmware is already loaded an error message will indicate this and the
firmware needs to be manually unloaded with the following command:
  nfp-nffw unload
  
-------------------------------------------------------------------------------
*** Installing SDK6 Run Time Environment and SDK6 simulator 
-------------------------------------------------------------------------------
This is the recommended installation procedure for using the SDK6 RTE with
SDK6 simulator on Linux. Simulator minimum requirement of 3GB of memory for 
running simple simulations, as simulation gets bigger memory requirement can go
as high as 9GB.

To install only the RTE without the NFP BSP:
  ./sdk6_rte_install.sh install_rte_only
  
Download the latest release tarball of the SDK simulator matching the RTE 
version.

Extract the tarball to a desired location, a directory nfp-sdk-<version> will be 
created into which all the files will be extracted. This directory will be SDK6
simulator installation directory, for example if simulator version 6.0.3 is 
extracted to /opt the installation directory will be /opt/nfp-sdk-6.0.3.

From the installation directory go to examples/nfsim and run make

Create symlinks to libnfp.so and libnfp_common.so in in the lib directory in the
installation directory to make naming compatible with NFP BSP library names:
cd /opt/nfp-sdk-6.0.3/lib
ln -s libnfp.so libnfp.so.3
ln -s libnfp_common.so libnfp_common.so.0

Before running the RTE the lib directory in installation directory should be
added to the dynamic linker paths and the environmental variable NETRODIR should
be set to the installation directory, example:
Create and ld nfsim.conf file: vi /etc/ld.so.conf.d/nfsim.conf
Add to the newly created conf file: /opt/nfp-sdk-6.0.3/lib
Run configure dynamic linker run-time bindings: ldconfig 
Set NETRODIR environmental variable:  NETRODIR=/opt/nfp-sdk-6.0.3

Start the simulator by running nfsim in the bin subdirectory before starting the
RTE in SIM MODE.

Please read the README located in the simulator installation directory for more
information on configuring and using the simulator.
  
All install script commands must be run with sudo or as root. 

-------------------------------------------------------------------------------
*** PCIe Quirk (ERR47) Kernel Patch
-------------------------------------------------------------------------------
IMPORTANT - READ BEFORE COPYING, INSTALLING OR USING KERNEL PATCHES

Linux kernels exhibit undesired behavior in PCIe configuration code. Netronome
submitted a fix to the kernel maintainers for this issue which has been accepted
into kernel version 4.5. As you may be using an older kernel version, at this
stage a patch needs to be applied to the kernel source code, or an already
patched kernel needs to be installed. For your convenience, packages containing
patched versions of the default kernels for various Ubuntu 14.04/16.04 LTS and
RHEL/CentOS variants are available on Netronome's support
site (http://support.netronome.com).

To install, transfer the package files to a subdirectory on your system, change
to that subdirectory, and enter:
   dpkg -i *.deb    (on Ubuntu systems), or:
   rpm -Uvh *.rpm   (on RHEL/CentOS systems).
Contact Netronome support if you require additional assistance.

Software for kernel versions can be found at https://www.kernel.org/.  In the
event Software cannot be retrieved, contact Netronome via support
site (http://support.netronome.com) for additional assistance in obtaining.

This kernel patch code is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details, a copy
of this license has been provided in the accompanying README file.

-------------------------------------------------------------------------------
*** Uninstalling SDK6 Run Time Environment and Hardware Debug server
-------------------------------------------------------------------------------
To uninstall BSP, Hardware Debug Server and the SDK6 RTE:
  ./sdk6_rte_install.sh uninstall_with_bsp

To uninstall just Hardware Debug Server and the SDK6 RTE:
  ./sdk6_rte_install.sh uninstall
  
All install script commands must be run with sudo or as root.
  
Some files might be left in /opt/netronome that might need to be removed 
manually, SDK6 simulator must be manually removed.

-------------------------------------------------------------------------------
*** Starting and stopping using Upstart (Ubuntu 14.04 and CentOS 6)
-------------------------------------------------------------------------------
The RTE (NORMAL MODE) can be started/stopped by calling:
  start/stop nfp-sdk6-rte
    
The RTE(DEBUG MODE) can be started/stopped by calling:
  start/stop nfp-sdk6-rte-debug
  
The RTE(SIM MODE) can be started by calling:
  start/stop nfp-sdk6-rte-sim 
Before using this Upstart configuration set NETRODIR to the SDK6 simulator
installation directory in the installed file nfp-sdk6-rte-sim.conf

The Hardware Debug Server can be started by calling:
  start/stop nfp-hwdbg-srv
    
To start the job at system ready uncomment the startup line in 
nfp-sdk6-rte.conf, nfp-sdk6-rte-debug.conf or nfp-hwdbg-srv.conf in /etc/init/.

To check whether the RTE job started correctly and is still running use:
  status nfp-sdk6-rte           (add -debug for DEBUG MODE or -sim for SIM MODE)
if the status show stop/waiting the RTE has stopped and an error probably 
occurred. Look at either the Upstart job log in 
/var/log/upstart/nfp-sdk6-rte.log (replace nfp-sdk6-rte with the job name you 
started) or look in /var/log/nfp-sdk-rte.log for RTE only logs. To have a 
continuous live log open either log with 
  tail -f <log_file_name>.log

-------------------------------------------------------------------------------
*** Starting and stopping using Systemd (Ubuntu 16.04 and CentOS/RHEL 7)
-------------------------------------------------------------------------------
The RTE (NORMAL MODE) can be started/stopped  by calling:
  systemctl start/stop nfp-sdk6-rte
    
The RTE(DEBUG MODE) can be started/stopped by calling:
  systemctl start/stop pnfp-sdk6-rte-debug
  
The RTE(SIM MODE) can be started/stopped by calling:
  systemctl start/stop nfp-sdk6-rte-sim
Before using this Upstart configuration set NETRODIR to the SDK6 simulator
installation directory in the file 
/usr/lib/systemd/system/nfp-sdk6-rte-sim.service
  
The Hardware Debug Server can be started/stopped by calling:
  systemctl start/stop nfp-hwdbg-srv
    
To start the programs at system startup run the systemctl enable command for the
specified service:
  systemctl enable nfp-sdk6-rte.service

To check whether the RTE service started correctly and is still running use:
  systemctl status nfp-sdk6-rte (add -debug for DEBUG MODE or -sim for SIM MODE)
if the Active status show inactive (dead) the RTE has stopped and an error 
probably occurred.  Look at either the Systemd journal or in the RTE logs for 
more detail on what error occurred.

For looking in the Systemd journal use the following command:
  journalctl -u nfp-sdk6-rte
replace nfp-sdk6-rte with the service name you are using (ex nfp-sdk6-rte-debug
or nfp-sdk6-rte-sim). Add the -f argument to follow the journal for a live log.

For only logs generated by the RTE look /var/log/nfp-sdk-rte.log, for continuous 
live log open the log with
  tail -f /var/log/nfp-sdk-rte.log

-------------------------------------------------------------------------------
*** Usage scenarios
-------------------------------------------------------------------------------
When full performance is required and if the user is not debugging applications
through breakpoints and code stepping vie the NFP SDK6 debugging interface 
start RTE in NORMAL MODE.

If the user is debugging application through code stepping and breakpoints run
the RTE in DEBUG MODE, this causes the firmware to run on a single ME to ease
debugging but will seriously limit performance.

If firmware is loaded via the NFP SDK6 interface the Hardware Debug Server also
needs to be started with the RTE, if firmware is loaded via an alternative 
interface or via the RTE CLI the Hardware Debug Server needs to stopped.

To load and debug application using the SDK6 simulator the simulator must be 
running on the target host machine/VM and the RTE must then be started on the 
same machine in SIM MODE.

-------------------------------------------------------------------------------
*** Starting and stopping NOT using Upstart or Systemd
-------------------------------------------------------------------------------
To directly start the RTE the binary can be located at: 
/opt/nfp_pif/bin/pif_rte
To start, a load script needs to specified:
  NORMAL MODE: pif_rte -z -s /opt/nfp_pif/scripts/pif_ctl_nfd.sh
  DEBUG MODE: pif_rte -z --sdk-debug -s /opt/nfp_pif/scripts/pif_ctl_nfd.sh
  SIM MODE: pif_rte -z -s /opt/nfp_pif/scripts/pif_ctl_sim.sh
  
Programmer Studio and the included Python CLI client both use zlib compression 
in the transport layer, if using a custom client not using zlib omit the -z 
input argument.
  
The location of the Netronome Board Support Package (NFP-BSP) libraries must be
added to LD_LIBRARY_PATH. Default location for these libraries will be  
/opt/netronome/lib

To directly start /opt/netronome/nfp-sdk-hwdbgsrv/server/nfp-sdk-hwdbgsrv

Run these applications with the -h or --help input argument for various options
and input arguments available for these applications. Appendix D in the NFP-6000
Development Tools User Guide that is packaged with Programmer Studio contains 
more detailed usage instructions.

-------------------------------------------------------------------------------
*** Server with multiple Intelligent Server Adapters (ISA's)
-------------------------------------------------------------------------------
When running multiple ISA's a separate instance of RTE must be started for each
device. If debugging a separate instance of Hardware Debug Server and Programmer
Studio should also be started as well. For RTE and Hardware Debug Server an 
unique unused port should be specified for each instance. Both port numbers and 
the hardware device number must set in the corresponding instance of Programmer 
Studio (Hardware->Options)

For both pif_rte and nfp-sdk-hwdbgsrv the hardware device number is specified 
with -n input argument and the remote procedure call port with -p.
Devices are indexed at system startup starting from 0, if device and PCIe setup
stays unchanged the device number will stay the same.
More info for each device can retrieved using:  
 /opt/netronome/bin/nfp-hwinfo -n <hardware_device_number>

Alternatively previously configured Upstart or systemd init scripts (installed
to the corresponding system init directories) can be used as described in the
above sections:
Service/job name          Hardware Device Number           Port
nfp-sdk6-rte(-debug)                0                       20206
nfp-sdk6-rte(-debug)1               1                       20207
nfp-sdk6-rte(-debug)2               2                       20208
nfp-sdk6-rte(-debug)3               3                       20209
nfp-hwdbg-srv                       0                       20406
nfp-hwdbg-srv1                      1                       20407
nfp-hwdbg-srv2                      2                       20408
nfp-hwdbg-srv3                      3                       20409

The log for when working with hardware device 0 is /var/log/nfp-sdk-rte.log,
for devices 1,2,3 the logs are /var/log/nfp-sdk-rte1.log, 
/var/log/nfp-sdk-rte2.log and /var/log/nfp-sdk-rte3.log respectively.

-------------------------------------------------------------------------------
*** Log files
-------------------------------------------------------------------------------
Output of the RTE will be logged to /var/log/nfp-sdk-rte.log

Output of the Hardware Debug Server will be logged to /var/log/nfp-hwdbg-srv.log

When using Upstart, output of the NFP hardware init and firmware loading script 
will be logged to /var/log/upstart/nfp-sdk6-rte.log or 
/var/log/upstart/nfp-sdk6-rte-debug.log depending on which mode is being run.

When using systemd, output of the NFP hardware init and firmware loading script 
will be logged to the systemd journal; use journalctl -u nfp-sdk6-rte or 
journalctl -u nfp-sdk6-rte-debug depending on which mode is being run