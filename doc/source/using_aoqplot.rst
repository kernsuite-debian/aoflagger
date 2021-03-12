Using ``aoqplot``
=================

``aoqplot`` is an interactive tool to browse through quality statistics, but also allows non-interactive access to plots that can e.g. be stored automatically to disk. These quality statistics are added to a measurement set by the :doc:`aoquality <using_aoquality>` tool or within certain pipelines (e.g. ``DPPP``, ``cotter``).

The normal way of running ``aoqplot`` is:

.. code-block:: bash
    
    aoqplot <ms1> [<ms2>..]
    
This will read the statistics from the given measurement set. If multiple measurement sets are specified, the statistics will be combined.

The following statement saves all standard deviation plots without user interaction:

.. code-block:: bash
    
    aoqplot observation.ms -save Obs StandardDeviation
    
The non-interactive options for running ``aoqplot`` are described on the page :doc:`scripted plotting <programs_scripting>`.
