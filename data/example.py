import aoflagger as aof
import numpy
print("Flagging with AOFlagger version " + aof.AOFlagger.get_version_string())

nch = 256
ntimes = 1000

aoflagger = aof.AOFlagger()

# Load strategy from disk (alternatively use 'make_strategy' to use a default one)
strategy = aoflagger.load_strategy("example-strategy.rfis")

data = aoflagger.make_image_set(ntimes, nch, 8)

print("Number of times: " + str(data.width()))
print("Number of channels: " + str(data.height()))

# When flagging multiple baselines, iterate over the baselines and
# call the following code for each baseline
# (to use multithreading, make sure to create an imageset for each
# thread)

# Make eight images: real and imaginary for 4 pol

for imgindex in range(8):
    # Initialize data
    values = numpy.zeros([nch, ntimes])
    data.set_image_buffer(imgindex, values)

flags = aoflagger.run(strategy, data)
flagvalues = flags.get_buffer()
flagcount = sum(sum(flagvalues))
print("Percentage flags on zero data: " + str(flagcount * 100.0 / (nch*ntimes)) + "%")

# Collect statistics
# We create some unrealistic time and frequency arrays to be able
# to run these functions. Normally, these should hold the time
# and frequency values.
timeArray = numpy.linspace(0.0, ntimes, num=ntimes, endpoint=False)
freqArray = numpy.linspace(0.0, nch, num=nch, endpoint=False)
qs = aoflagger.make_quality_statistics(timeArray, freqArray, 4, False)
aoflagger.collect_statistics(qs, data, flags, aoflagger.make_flag_mask(ntimes, nch, False), 0, 1)
try:
    aoflagger.write_statistics(qs, "test.qs")
except:
    print("write_statistics() failed")
