Using ``aoquality``
===================

The ``aoquality`` program is a command-line tool to query or modify the quality statistics in the table. It is more or less a command-line counterpart to :doc:`aoqplot <using_aoqplot>`.

What quality statistics are
---------------------------

.. default-domain:: lua

Quality statistics are optional tables in a measurement set that describe statistics of the visibility in the measurement set. These can be computed from the visibilities and are therefore redundant, but they are much smaller in size, and therefore alleviate the need to read an entire set just to get some first idea of the quality of the visibilities. They can be calculated during flagging (using the lua :meth:`~aoflagger.collect_statistics` function), which avoids reading the data again just for getting the statistics.

.. note::
    If the visibilities are changed, the quality statistics become out of date. This can be desirable in some cases, e.g. to keep the statistics of the raw, high-resolution data before averaging.

(Re)collecting statistics
-------------------------

To collect statistics, the following command can be used:

.. code-block:: bash

    aoquality collect [-d <column name>] observation.ms
    
This will read all the visibilities and add the quality statistics tables to the measurement set afterwards. If quality tables already exist, they are overwritten. If no column name is specified, the ``DATA`` column is used.

Querying statistics
-------------------

There are several dimensions over which statistics, such as the standard deviation, are stored and can be queried: over time (integrated over all other dimensions), frequency or baseline. To query these, the ``aoquality`` tool accepts these commands:

  * ``aoquality query_b``: Query the statistics per baseline (integrated over all other dimensions)
  * ``aoquality query_f``: Query the statistics per channel (integrated over all other dimensions)
  * ``aoquality query_t``: Query the statistics over time (integrated over all other dimensions)
  * ``aoquality query_g``: Query a statistic integrated over all dimensions
  
Each of these commands uses the same syntax. We will use ``query_f`` as an example. To query the standard deviation per channel, one would issue this command:

.. code-block:: bash

    aoquality query_f StandardDeviation observation.ms
    
As shown, after ``query_f`` the type of statistic is given, followed by the name of the measurement set. A few other common statistics are ``Mean``, ``Variance``, ``Count`` (=nr of visibilities), ``RFIRatio`` and ``DStandardDeviation`` (=frequency-differenced standard deviation).

Here is an example output:

.. code-block::

  FREQUENCY StandardDeviation_POL0_R StandardDeviation_POL0_I StandardDeviation_POL1_R StandardDeviation_POL1_I StandardDeviation_POL2_R StandardDeviation_POL2_I StandardDeviation_POL3_R StandardDeviation_POL3_I
  146.852 0.00388077      0.00138652      0.0085672       0.00149141      0.00472147      0.000928892     0.00356818      0.00108439
  146.862 0.00408367      0.0035652       0.00532761      0.00300345      0.00263579      0.00237315      0.00415023      0.00305888
  146.872 0.0037208       0.00351583      0.00158907      0.00157247      0.00150166      0.00152535      0.00399859      0.00298128
  146.882 0.00386623      0.00362623      0.00158918      0.00158995      0.00151442      0.00152671      0.00409885      0.00304795
  146.892 0.00403239      0.00368274      0.00160513      0.00159917      0.00153444      0.00153748      0.00416647      0.00306265

This shows the frequency in the first column, the standard deviation of the real values of XX in the second column, followed by imaginary, then real of XY, etc.
  
Other options
-------------

The ``aoquality`` command has a few more options. These can be queried by running ``aoquality`` without parameters.
