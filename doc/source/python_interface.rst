Python interface
================

The external Python API mirrors the external C++ API, and only differs in that it follows
the common Python naming conventions. For a reference of the functions and a general
overview of what can be done with the Python interface, see the
:doc:`C++ interface <cpp_interface>`. This chapter discusses a few practical topics
related to the Python interface.

Installation
^^^^^^^^^^^^

The AOFlagger Python module is compiled into an object library. It is compiled
along when you run ``make`` as described on the :doc:`installation` chapter.
Currently, make compiles the following library on
my computer:

.. code-block:: text

   build/python/aoflagger.cpython-39-x86_64-linux-gnu.so

The normal/proper way of installing this library into its correct location, is by
running ``make install``. On my computer, this copies that file to
``/install-prefix/lib``. For Python to find this library, the path needs to
be in your Python search path, which is normally set with the 
``PYTHONPATH`` environment variable, e.g.:

.. code-block:: bash

   export PYTHONPATH=/install-prefix/lib:${PYTHONPATH}
   
There's no need to run a ``setup.py`` for AOFlagger.
Also, AOFlagger can't be installed via pip. When installing AOFlagger
via a Debian/Ubuntu package, the library should be installed and found
without any manual user tweaking. Be aware that the Python interface
and binding tool was improved in version 3.0, and it is therefore recommended
that the latest release of aoflagger (>= 3.0) is used.
   
Using the Python interface
^^^^^^^^^^^^^^^^^^^^^^^^^^

The aoflagger module can be included in
Python using a standard ``import`` command:

.. code-block:: python

    import aoflagger
   
A few examples are given in the ``data`` directory. The following is an example to calculate
the false-positives ratio of the default strategy:

.. code-block:: python

    import aoflagger
    import numpy

    nch = 256
    ntimes = 1000
    count = 50       # number of trials in the false-positives test

    flagger = aoflagger.AOFlagger()
    path = flagger.find_strategy_file(aoflagger.TelescopeId.Generic)
    strategy = flagger.load_strategy_file(path)
    data = flagger.make_image_set(ntimes, nch, 8)

    ratiosum = 0.0
    ratiosumsq = 0.0
    for repeat in range(count):
        for imgindex in range(8):
            # Initialize data with random numbers
            values = numpy.random.normal(0, 1, [nch, ntimes])
            data.set_image_buffer(imgindex, values)
            
        flags = strategy.run(data)
        flagvalues = flags.get_buffer()
        ratio = float(sum(sum(flagvalues))) / (nch*ntimes)
        ratiosum += ratio
        ratiosumsq += ratio*ratio

    print("Percentage flags (false-positive rate) on Gaussian data: " +
        str(ratiosum * 100.0 / count) + "% +/- " +
        str(numpy.sqrt(
            (ratiosumsq/count - ratiosum*ratiosum / (count*count) )
            ) * 100.0) )

This takes about 10 seconds to run on my computer.

