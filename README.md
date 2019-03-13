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
	

## Installing SDK

Follow [instructions provided by EDT](https://edt.com/pdv_run_installation_instructions/)

Installation requires kernel sources:

    sudo apt-get update
    sudo apt-get install linux-source
    sudo apt-get install linux-headers-generic

Installation directory is /opt/EDT/ on scexao2 computer (kernel 3.13.0-144-generic)



## Installing EDT PDV software for Linux

Download the files from (https://edt.com/pdv_run_installation_instructions/). Right-click on the link, then select “Save link” from the drop-down menu.
 
Save the EDTpdv_lnx_5.5.5.1.run file to some temporary directory. Then change to that directory and run:

    chmod +x EDTpdv_lnx_5.5.5.1.run
    sudo ./EDTpdv_lnx_5.5.5.1.run

Select default install directory when prompted (/opt/EDTpdv).
Install takes a couple of minutes.

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

## Compilation

Source code is in ./src/directory.
To compile:

	gcc -I/opt/EDTpdv imgtake.c /opt/EDTpdv/libpdv.a -lm -lpthread -ldl -o imgtake


# Application

Application programs are ircamserver and imgtake. To compile:

	cd ./src
	./compile

