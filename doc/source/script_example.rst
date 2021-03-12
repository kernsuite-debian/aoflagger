Example script
==============

.. default-domain:: lua

This section demonstrates a simple example script.
Copying the script below into the ``rfigui`` Lua editor will allow you to
interactively change this script. 

.. code-block:: lua

    function execute(data)

      data:clear_mask()
      
      for _,polarization in ipairs(data:get_polarizations()) do

        local pol_data = data:convert_to_polarization(polarization)
        pol_data = pol_data:convert_to_complex("amplitude")

        aoflagger.high_pass_filter(pol_data, 51, 51, 3, 3)
        aoflagger.visualize(pol_data, "Low-pass filtered", 0)
        
        aoflagger.sumthreshold(pol_data, 1, 1, true, true)
        aoflagger.scale_invariant_rank_operator(pol_data, 0.2, 0.2)

        pol_data = pol_data:convert_to_complex("complex")
        data:set_polarization_data(polarization, pol_data)
        
      end -- end of polarization iterations

    end

We will discuss this script line by line.
When AOFlagger calls the ``execute()`` function, the script starts by calling
:meth:`Data.clear_mask`, which unsets all flags:

.. code-block:: lua

    function execute(data)

      data:clear_mask()
      
If the input data was already flagged, these flags are removed.

The next step is a ``for`` loop over the different polarizations in the set:

.. code-block:: lua

      for _,polarization in ipairs(data:get_polarizations()) do
      
Almost any flagging script will need such a loop, because most
operations can only work on a single polarization. The input data could consist
of 1-4 linear polarizations (XX, XY, YX, YY), circular polarizations
(LL, LR, RL, RR) or Stokes polarizations (I, Q, U, V).
:meth:`Data.get_polarizations()` returns a table of strings, and ``ipairs``
converts this to an iterator. A loop over a table captures an
(index, value) pair. In this case we only need the value (``polarization``),
and ignore the index with the underscore.

To work on a single polarization only, :meth:`Data.convert_to_polarization` is
used to create a new data object that contains just that single polarization:

.. code-block:: lua

        local pol_data = data:convert_to_polarization(polarization)

The new object is stored as ``local`` variable. In Lua, any variable that is
not declared as ``local``, is a global. This might cause the data to be stored
longer than necessary, causing more memory usage, so local variables should be
preferred. [1]_

Most input data sets are complex. Thresholding is more effective on the
amplitudes of the samples though. Function ``Data.convert_to_complex`` is used
to calculate the amplitudes:

.. code-block:: lua
        
        pol_data = pol_data:convert_to_complex("amplitude")

Another new object is created, but the previous ``pol_data`` object is
overwritten. 

The next step is high-pass filtering the data:

.. code-block:: lua
        
        aoflagger.high_pass_filter(pol_data, 51, 51, 3, 3)

Function :meth:`aoflagger.high_pass_filter` filters the visibility data in
`pol_data`. A kernel of 51 x 51 samples is used (ntimes x nchannels), with a
Gaussian width of 3 samples in both directions.

The filtered data is "visualized" with function :meth:`aoflagger.visualize`:

.. code-block:: lua
        
        aoflagger.visualize(pol_data, "Low-pass filtered", 0)

This statement makes it possible to display the result of filtering the
data in ``rfigui``. When the script is not running interactively from a
gui, the call is ignored. Note that ``visualize()`` will be called for
all polarizations. The gui will recombine visualizations from different
polarizations, as long as they have the same name and sorting index.

The :meth:`aoflagger.sumthreshold` searches the (filtered) data for consecutive high
values in time or frequency:

.. code-block:: lua

        aoflagger.sumthreshold(pol_data, 1, 1, true, true)
        
The resulting ``pol_data`` object will now contain a flag mask. This flag mask
is morphologically extended in the time and frequency direction with the
:meth:`aoflagger.scale_invariant_rank_operator` function:

.. code-block:: lua

        aoflagger.scale_invariant_rank_operator(pol_data, 0.2, 0.2)

Finally, the data are converted back to its original form, so that the
polarizations can be combined. The first step is to convert the data back
to complex values using :meth:`Data.convert_to_complex`:

.. code-block:: lua

        pol_data = pol_data:convert_to_complex("complex")

This "conversion" can be seen as an update of the metadata. The phases
were lost while converting to amplitudes, so these are not restored, and
just assumed to be constant. The ``pol_data`` now holds a complex data and
our new flag mask. The last step is to update the input data with our
new data using :meth:`Data.set_polarization_data`:
        
.. code-block:: lua

        data:set_polarization_data(polarization, pol_data)

By iterating over all polarizations in the input set, the
polarizations in the input data object are replaced one by one.
        
.. [1] Data objects are emptied by AOFlagger at the end of the ``execute()``
    function, so the user should normally not worry about memory usage.

