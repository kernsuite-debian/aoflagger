import aoflagger as aof
import matplotlib.pyplot as plt
import numpy
import sys

nch = 256
ntimes = 1000
npol = 2

aoflagger = aof.AOFlagger()
strategy = aoflagger.make_strategy(aof.TelescopeId.Generic, 0, 150e6, 1, 4e3)
data = aoflagger.make_image_set(ntimes, nch, npol*2)

# Several consecutive values at the same frequency are increased
# in amplitude to simulate a RFI source. These values define
# the channel and the start and duration of the added signal.
rfi_y = int(nch*0.3)
rfi_x_start = int(ntimes*0.2)
rfi_x_end = int(ntimes*0.4)
rfi_strength = 1 # 1 sigma above the noise

for imgindex in range(npol*2):
    # Initialize data with random numbers
    values = numpy.random.normal(0, 1, [nch, ntimes])
    # Add fake transmitter
    values[rfi_y,rfi_x_start:rfi_x_end] = values[rfi_y,rfi_x_start:rfi_x_end] + rfi_strength
    data.set_image_buffer(imgindex, values)
    

flags = aoflagger.run(strategy, data)
flagvalues = flags.get_buffer()
flagvalues = flagvalues*1

plt.imshow(values, cmap='viridis')
plt.colorbar()
plt.show()

plt.imshow(flagvalues, cmap='viridis')
plt.colorbar()
plt.show()
