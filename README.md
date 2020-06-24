# hardw-cred2

milk interface to C-RED 2 camera

Using EDT frame grabber Camera Link frame grabber for PCI Express / x4.

See vendor [product link](https://edt.com/product/visionlink-f4/) and [SDK](https://edt.com/file-category/pdv/)


## Directories and files

	# EDT driver install
	/home/scexao/src/EDTpdv_lnx_5.5.5.1.run
	
	# working directory (all commands executed from here)
	/home/scexao/src/hardw-cred2

	# SDK install directory
	/opt/EDTpdv/
	

## Deps

Installation requires kernel sources:

    sudo apt-get update
    sudo apt-get install linux-source
    sudo apt-get install linux-headers-generic


## Installing EDT PDV software for Linux

Download the files from (https://edt.com/pdv_run_installation_instructions/). Right-click on the link, then select “Save link” from the drop-down menu.
 
Save the EDTpdv_lnx_5.5.5.1.run file to some temporary directory. Then change to that directory and run:

    chmod +x EDTpdv_lnx_5.5.5.1.run
    sudo ./EDTpdv_lnx_5.5.5.1.run

Select default install directory when prompted (/opt/EDTpdv).
Install takes a couple of minutes.


#### Provisional bugfix

Note: Ubuntu 20.04, kernel 5.4.0, the install wouldn't work, failing at the module compilation and with a less than helpful error message telling to check if linux sources are installed. Checking the `make` log point to:

	No rule to make target 'arch/x86/tools/relocs_32.c', needed by 'arch/x86/tools/relocs_32.o'.  Stop.
	
This can be fixed, going in `/opt/EDTpdv/module/Makefile` and amending (line 90):

	$(MAKE) -C $(KDIR) V=1 SUBDIRS=$(CURDIR) modules # 1>make.output
	into
	$(MAKE) -C $(KDIR) M=$(shell pwd) V=1 SUBDIRS=$(CURDIR) modules # 1>make.output
	
The magic involved here is beyond me, but it worked.
Complete the installation by running, `/opt/EDTpdv`:

	sudo setup.sh
	
####  Post checks

After installation, the kernel module edt should be loaded. Command:

	lsmod | grep edt

returns:

	edt                   200773  0

The following device should also appear:

	/dev/edt

[Device configuration guide](https://edt.com/downloads/ad_config_guide/)



## If the kernel version changes

Changes in kernel version will prevent the kernel module edt.ko to be loaded. The driver will need to be re-installed and the kernel module rebuilt:

	# run from /opt/EDTpdv/
	sudo ./uninstall.sh
	
	# re-install drivers and kernel module (see previous section)

Alternatively:

	# Navigate to /opt/EDTpdv
	make unload
	make driver load

## Hardware Initialization of Camera

	/opt/EDTpdv/initcam -c 0 -f config/cred2_FGSetup_14bit.cfg 



# Application program

Running the camera requires running two process: server (ircamserver) and image acquisition (imgtake).

## Compile 

Application programs are ircamserver and imgtake. To compile:

	cd ./src
	./compile

## Running 
