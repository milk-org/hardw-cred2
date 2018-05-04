# hardw-cred2
milk interface to C-RED 2 camera

Using EDT frame grabber Camera Link frame grabber for PCI Express / x4.

See vendor [product link](https://edt.com/product/visionlink-f4/) and [SDK](https://edt.com/file-category/pdv/)


## Installing SDK

Follow [instructions provided by EDT](https://edt.com/pdv_run_installation_instructions/)

Installation requires kernel sources:

    sudo apt-get update
    sudo apt-get install linux-source
    sudo apt-get install linux-headers-generic

Installation directory is ~/src/EDT/ on scexao2 computer (kernel 3.13.0-43-generic)

## Installing EDT PDV software for Linux
Download the files from (https://edt.com/pdv_run_installation_instructions/).Right-click on the link, then select “Save link” from the drop-down menu. 


[Device configuration guide](https://edt.com/downloads/ad_config_guide/)


## Hardware Initialization of Camera

	/opt/EDTpdv/initcam -c 0 -f cred2_FGSetup_14bit.cfg 

## Compilation

Source code is in ./src/directory.
To compile:

	gcc -I/opt/EDTpdv imgtake.c /opt/EDTpdv/libpdv.a -lm -lpthread -ldl -o imgtake


