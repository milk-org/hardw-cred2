from subprocess import call
output = call("/opt/EDTpdv/initcam -c 0 -f /opt/EDTpdv/camera_config/cred2_FGSetup_14bit.cfg", shell=True)

from edtlib import *
edt = EdtIF()
import matplotlib.pyplot as plt
import numpy as np

#edt.setExposureTime(0.1)
#edt.getImageSize()
a=edt.getLastImage();
an=np.array(a[0]).astype('float'); 
b=plt.imshow(an.reshape(512,640)); 
plt.colorbar()
plt.show()

