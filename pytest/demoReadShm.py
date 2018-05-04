import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
import numpy as np
import shmlib

shmimName = "/tmp/cred2_transpose.im.shm"


shmim = shmlib.shm(shmimName)
    
im=shmim.get_data().reshape(640,512)    #DEFLAG: changed to 640,512 vs. 512,640
b=plt.imshow(im); 
plt.ion()
plt.colorbar()
plt.show()
for k in range(0,100):
    im=shmim.get_data().reshape(640,512)    #DEFLAG: changed to 640,512 vs. 512,640
    b=plt.imshow(im); 
    plt.pause(0.01)

