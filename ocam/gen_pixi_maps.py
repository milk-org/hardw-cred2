import numpy as np
import astropy.io.fits as pf

# What format do we want ?
# We want a 32bit map (fits) of the size the dump of the FG, at which each pixel contains the index of the pixel it has to go into
# in the final image
# Note that
# Ocam input is little endian -> we want big endian OR is it big endian ??? Manuals UNCLEAR FFS
# We're gonna run that in pure 8bit, so ocam sizes are 240x480 and 120x240

# Generate the unbinned map
arange = np.arange(121*66, dtype=np.int32)
# Move the +1 here to change the enddianness
amps = [(np.c_[16*arange+(1-i%2), 16*arange+(i%2)] + 2*i).reshape(121, 132) for i in range(8)]

# Crop the prescans
amps = [a[1:,12:] for a in amps]

# Gen the sensor map with readout order
sensor = np.r_[np.c_[amps[0][:,::-1], amps[1],
                     amps[2][:,::-1], amps[3]],
               np.c_[amps[7][:,::-1], amps[6],
                     amps[5][:,::-1], amps[4]][::-1]
              ]
print("240x480?", sensor.shape)

buff = np.zeros((121*1056), dtype=np.int32)
for k,v in enumerate(sensor.flatten()):
    buff[v] = k
buff = buff.reshape(121, 1056)

pf.writeto('ocam2kpixi_18.fits', buff, overwrite=True)


# Now same for the 120x120 - now there is one prescan line and one postscan line entirely
# Also, we know the pixels will be duplicated - EACH sequence of 8 pixels across the 8 amps is repeated twice
arange = np.arange(62*66, dtype=np.int32)

amps = [(np.c_[16*arange+(1-i%2), 16*arange+i%2] + 2*i).reshape(62, 132) for i in range(8)]
# Remove the prescans
amps = [a[1:-1,12:] for a in amps]
# Remove the double readouts - keeping two bytes every four
amps = [a.reshape(60,30,4)[:,:,:2].reshape(60,60) for a in amps]
sensor = np.r_[np.c_[amps[0][:,::-1], amps[1],
                     amps[2][:,::-1], amps[3]],
               np.c_[amps[7][:,::-1], amps[6],
                     amps[5][:,::-1], amps[4]][::-1]
              ]

print("120x240?", sensor.shape)

buff = np.zeros((62*1056), dtype=np.int32)
for k,v in enumerate(sensor.flatten()):
    buff[v] = k
buff = buff.reshape(62, 1056)

pf.writeto('ocam2kpixi_38.fits', buff, overwrite=True)


# Try again - lets generate the full and binned 16bit maps

# Full
# Generate the unbinned map
arange = np.arange(121*66, dtype=np.int32)
# Move the +1 here to change the enddianness
amps = [8 * arange.reshape(121,66) + i for i in range(8)]

# Crop the prescans
amps = [a[1:,6:] for a in amps]

# Gen the sensor map with readout order
sensor = np.r_[np.c_[amps[0][:,::-1], amps[1],
                     amps[2][:,::-1], amps[3]],
               np.c_[amps[7][:,::-1], amps[6],
                     amps[5][:,::-1], amps[4]][::-1]
              ]
print("240x240?", sensor.shape)

buff = np.zeros((121*528), dtype=np.int32)
for k,v in enumerate(sensor.flatten()):
    buff[v] = k
buff = buff.reshape(121, 528)

pf.writeto('ocam2kpixi_1.fits', buff, overwrite=True)

# Binned
# Generate the unbinned map
arange = np.arange(62*66, dtype=np.int32)
# Move the +1 here to change the enddianness
amps = [8 * arange.reshape(62,66) + i for i in range(8)]

# Crop the prescans
amps = [a[1:-1,6:] for a in amps]
# Remove the duplicate reads
amps = [a[:,::2] for a in amps]

# Gen the sensor map with readout order
sensor = np.r_[np.c_[amps[0][:,::-1], amps[1],
                     amps[2][:,::-1], amps[3]],
               np.c_[amps[7][:,::-1], amps[6],
                     amps[5][:,::-1], amps[4]][::-1]
              ]
print("120x120?", sensor.shape)

buff = np.zeros((62*528), dtype=np.int32)
for k,v in enumerate(sensor.flatten()):
    buff[v] = k
buff = buff.reshape(62, 528)

pf.writeto('ocam2kpixi_3.fits', buff, overwrite=True)

