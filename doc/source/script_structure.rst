Execute and options reference
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. default-domain:: lua

The formal specification for the ``execute()`` and ``options()`` functions
is as follows.

.. function:: execute(data)

   This is the main flagging function that is to be implemented by the flagging
   script. AOFlagger will look for the global ``execute`` function, and call
   this when data is available to be flagged. It is possible to give
   this function a different name, or have multiple functions by changing
   the defaults in :meth:`options`.
   
   On input, the ``data`` option contains the visibility data and the current
   flag mask. The object's interface is described by the :class:`Data` class.
   The task of the ``execute`` function is to modify this data
   object.
   
   The command line :doc:`aoflagger <using_aoflagger>` program will go through a measurement
   set, call ``execute`` for each baseline and save the resulting
   flag mask. The visibilities themselves are not written back to the
   measurement set, so any changes to them (e.g. by
   :meth:`~aoflagger.low_pass_filter`)
   will not change the data on disk.
   
   It is fine for the ``execute`` function to set global parameters.
   Note however that, when
   ``execute`` is called by ``aoflagger``, different calls to ``execute`` might
   run within a different Lua context. Every thread is assigned
   its own Lua context.

   :param data: Single-baseline data to be flagged
   :type data: :class:`Data`

.. function:: options()

   This special function is called by ``aoflagger`` to obtain options that are
   relevant for the strategy (or strategies).
   
   It should return a table for which each entry in the table maps
   a label to an option list. An option list is a table itself,
   mapping option names to their values.
   Any option that is not set, is left to its default.
   
   Options on the command line override options that are set by the strategy
   for all runs defined.
   The ``options`` function is optional: if it is not defined, all options
   are left to their defaults. Available options can be found on the
   :doc:`list of options <strategy_options>`.
   
   :returns: An option list
   :rtype: Table
   
