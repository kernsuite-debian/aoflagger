import pytest
import sys
import aoflagger
import numpy

# Append current directory to system path in order to import testconfig
sys.path.append(".")
import testconfig as config


"""
Use AOFlagger's Python interface to perform a simple flagging of Gaussian data.
This test does not use an initial mask.
"""


def test_without_initial_mask():
    n_channels = 256
    n_times = 400
    n_polarizations = 4
    n_images_per_polarization = 2  # Real and imaginary
    n_images = n_polarizations * n_images_per_polarization

    flagger = aoflagger.AOFlagger()
    path = flagger.find_strategy_file(aoflagger.TelescopeId.Generic)
    strategy = flagger.load_strategy_file(path)
    data = flagger.make_image_set(n_times, n_channels, n_images)

    for imgindex in range(n_images):
        # Initialize data with random numbers
        values = numpy.random.normal(0, 1, [n_channels, n_times])
        data.set_image_buffer(imgindex, values)

    flags = strategy.run(data)
    flagvalues = flags.get_buffer()
    ratio = float(sum(sum(flagvalues))) / (n_channels * n_times)

    # This tests the false positive ratio, which should be around 0.5%.
    assert ratio < 0.02

    # With so many samples, always at least a few samples should get flagged:
    assert ratio > 0


def test_set_mask():
    flagger = aoflagger.AOFlagger()
    n_channels = 100
    n_times = 50
    flag_mask = flagger.make_flag_mask(n_channels, n_times, False)
    buf = flag_mask.get_buffer()
    assert not buf.any()

    buf[10, 20] = True
    flag_mask.set_buffer(buf)
    buf = flag_mask.get_buffer()
    assert sum(sum(buf)) == 1
    assert buf[10, 20] == True
