Overview of programs
====================

AOFlagger provides several programs.
The :doc:`rfigui program <using_rfigui>` makes it possible to perform a "baseline by baseline" analysis of measurement sets
(and other supported formats). When analyzing data from a telescope for the first time, it is recommended
to first analyze the data with the ``rfigui``, and use it to test and tweak flagging strategies, or if
necessary to use it to :doc:`design your own <designing_strategies>`.

The :doc:`aoflagger program <using_aoflagger>` runs strategies in a non-interactive, automated fashion.
It is a command line program that, unlike ``rfigui``, does not require a graphical terminal. It applies
a flagging strategy to a dataset, and is optimized to run as fast as possible.
``aoflagger`` is therefore typically used to flag datasets "in production",
once a good flagging strategy has been found with the ``rfigui``.

After flagging, :doc:`aoqplot <using_aoqplot>` can be
used to inspect the results. 

If the strategy works well for a number of observations, it might be desirable to integrate the flagging
into an observational pipeline. This could involve calling ``aoflagger`` during the processing, but
sometimes it might be desirable to have a tighter coupling, e.g. to keep data in memory. For this, the
:doc:`C++ interface <cpp_interface>` or :doc:`Python interface <python_interface>`
can be used. Another reason to use the API is to run a flagging strategy on a
dataset with a format that is not natively supported by ``aoflagger``, e.g. a numpy array. 
