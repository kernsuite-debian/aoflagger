Using ``aoflagger``
===================

The '``aoflagger``' executable can be used to flag a measurement set.
It is a stand-alone command-line tool, i.e., without a graphical interface, and is designed to work
fast and non-interactively.

To flag a measurement set:

.. code-block:: bash

    aoflagger [options] <observation.ms>

The list of options can be retrieved by running ``aoflagger`` without parameters.

.. _reading modes:

Reading modes
-------------

The AOFlagger has several reading modes for opening measurement sets:

 * Direct mode, enabled with ``-direct-read``. This reads data when it is necessary. This results in scanning through the measurement set a lot, which can be very slow.
 * Reordering mode, enabled with ``-reorder``. Reorders the measurement set to disk, which is reasonably fast, but requires disk space. This used to be called 'indirect' mode.
 * Memory mode, enabled with ``-memory-read``: fastest mode, but only possible for sets that fit in memory.
 * Automatic mode, enabled with ``-auto-read-mode``: selects either reordering or memory mode based on available memory. This is the default.

.. note::

    Before AOFlagger version 3.3, the automatic reading mode selected the direct reading mode instead of the reordering mode when not enough memory is available.

The reordering mode
-------------------

This approach requires the size of the measurement set in free disk space (this would be the uncompressed size in case the measurement set is compressed with Dysco). AOFlagger will use reordering by specifying the "-reorder" parameter. In this case, the measurement set will be rewritten to a temporary location. Here is an example how to run AOFlagger in this mode:

.. code-block:: bash

    lce032:/data/offringa/temp$ aoflagger -reorder SB4.MS

Please note that the current working directory will be used as a temporary storage location! Thus by running ``aoflagger`` as in the above command, temporary files will be created in ``/data/offringa/temp``, and these will take up a volume equal to the size of the measurement set. Using fast storage (e.g. SSD drives or RAID setups) as temporary location will speed up AOFlagger with reordering considerably.

If you specify ``-v`` on the command line, you will see when the file is reordered. Here is an example output of ``aoflagger`` during initializing, when using the reordering mode: ::

    [..]
    0% : +-+-+-For each baseline...
    Estimate of memory each thread will use: 1 MB.
    Will process 91 baselines.
    0% : +-+-+-+-Initializing...
    Initializing observation times...
    Opening temporary files.
    Pre-allocating 16 MB...
    Pre-allocating 2 MB...
    Reordering data set...
    Done reordering data set of 18 MB in 0.917 s (19.9788 MB/s)
    [..]

If AOFlagger were to crash, please be sure to remove the temporary files (and send me a bug report if it is a bug).

