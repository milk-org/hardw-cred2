from subprocess import call
output = call("/opt/EDTpdv/initcam -c 0 -f /opt/EDTpdv/camera_config/cred2_FGSetup_14bit.cfg", shell=True)

from edtlib import *
import matplotlib.pyplot as plt
import numpy as np
import shmlib
import time
shmimName = "/tmp/cred2_transpose.im.shm"

edt = EdtIF()
time.sleep(0.5)
imv=edt.getLastImage();
im=np.array(imv[0]).astype('float'); 
shmim = shmlib.shm(shmimName, im.reshape(512,640).transpose()) 	#DEFLAG: transposed image to save correctly for chuckcam
time.sleep(0.5)

#for k in range(0,100):
try:
    while 1:
        imv=edt.getLastImage();
        im=np.array(imv[0]).astype('float')
        shmim.set_data(im.reshape(512,640).transpose())     #DEFLAG: transposed image to save correctly for chuckcam
        sys.stdout.write('\rNew Image %d \t'%(shmim.get_counter()))
        sys.stdout.flush()
        time.sleep(0.5)
except KeyboardInterrupt:
        edt.stopExposure()

#    b=plt.imshow(an.reshape(512,640)); 
#    plt.colorbar()
#    plt.show()

