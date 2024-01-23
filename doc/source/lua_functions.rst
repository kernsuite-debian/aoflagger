Functions
=========

This section lists all the Lua functions provided by AOFlagger.

.. default-domain:: lua

Summary
^^^^^^^
.. module:: aoflagger

The ``aoflagger`` module provides the following functions:

* Reporting and user interface:
    - :meth:`print_polarization_statistics`
    - :meth:`require_max_version`
    - :meth:`require_min_version`
    - :meth:`save_heat_map`
    - :meth:`set_progress`
    - :meth:`set_progress_text`
    - :meth:`visualize`

* Mathematical operations
    - :meth:`norm`
    - :meth:`sqrt`

* Data scaling:
    - :meth:`apply_bandpass`
    - :meth:`normalize_bandpass`
    - :meth:`normalize_subbands`
    
* Filtering and resolution:
    - :meth:`downsample`
    - :meth:`high_pass_filter`
    - :meth:`low_pass_filter`
    - :meth:`upsample_image`
    - :meth:`upsample_mask`
    
* Thresholding:
    - :meth:`sumthreshold`
    - :meth:`sumthreshold_masked`
    - :meth:`threshold_channel_rms`
    - :meth:`threshold_timestep_rms`

* Morphological:
    - :meth:`scale_invariant_rank_operator`
    - :meth:`scale_invariant_rank_operator_masked`

* Ranges
    - :meth:`copy_to_channel`
    - :meth:`copy_to_frequency`
    - :meth:`trim_channels`
    - :meth:`trim_frequencies`
    
* Other:
    - :meth:`collect_statistics`
    
Detailed descriptions
^^^^^^^^^^^^^^^^^^^^^

.. function:: apply_bandpass(data, filename)

   Apply a bandpass file to the data. The data is changed in place. Each line
   in the file contains <antenna name> <X/Y polarization>
   <channel index> <gain>, separated by spaces, for example:
   
   | RT2 X 0 0.7022
   | RT2 X 1 0.7371
   | RT2 X 2 0.8092
   | ...

   :param data: Data to which the bandpass is applied. 
   :type data: :class:`Data`
   :param filename: Path to bandpass textfile.
   :type filename: string

.. function:: collect_statistics(after_data, before_data)

   Calculate statistics, such as visibility standard deviation and flag
   percentages. When running the strategy on a measurement set, the statistics
   are stored inside the measurement set after finishing all baselines. These
   can be inspected by the ``aoqplot`` tool.
   
   The function takes the data after and before flagging. Any data that are
   flagged in ``before_data`` will not contribute to the statistics. This
   avoids counting e.g. correlator faults or shadowing as interference.

   :param after_data: Flagged data.
   :type after_data: :class:`Data`
   :param before_data: Unflagged data.
   :type before_data: :class:`Data`

.. function:: copy_to_channel(destination_data, source_data, channel)
   
   Copy the data (visibilities & flags) from the source data to 
   the destination data with a specified channel offset. This function
   can be used together with :meth:`trim_channels` to flag a subset of
   the data and copy the result back to the full data.
   
   In that scenario, the start channel for :meth:`trim_channels`
   equals the channel parameter in this call.
   
   When the source does not fit into the destination at the specified
   offset, only the part that fits is copied.
   
   Available since :doc:`v3.1 <changelogs/v3.01>`.
   
   :param destination_data: Destination data
   :type destination_data: :class:`Data`
   :param source_data: Data to be copied. These data are unchanged.
   :type source_data: :class:`Data`
   :param channel: A channel index that specifies the offset to which the
      data is copied in the destination data. 

.. function:: copy_to_frequency(destination_data, source_data, frequency)

   This function is similar to :meth:`copy_to_channel`, but instead of
   specifying the target channel offset as an index, it is specified as
   a frequency. This can be used as counterpart to
   :meth:`trim_frequencies`.
   
   See :meth:`copy_to_channel` for further info.
   
   Available since :doc:`v3.1 <changelogs/v3.01>`.
   
   :param destination_data: Destination data
   :type destination_data: :class:`Data`
   :param source_data: Data to be copied. These data are unchanged.
   :type source_data: :class:`Data`
   :param channel: A frequency in MHz that specifies the offset to which the
      data is copied in the destination data. 
   
.. function:: downsample(data, xfactor, yfactor, masked)

   Decrease the resolution of the data using simple linear binning. This can
   be effective to increase the speed of data smoothing, for example when using
   :meth:`high_pass_filter`. At the function end of :meth:`execute`,
   the data should have the original size. Therefore, a call to downsample
   should normally be followed by a call to :meth:`upsample_image` or
   :meth:`upsample_mask` to restore the
   visibilities and flags, respectively, to their original resolution.
   
   When the input data is not exactly divisable by the downsampling factors,
   fewer samples will be averaged into the last bins.

   :param data: Input data (not modified).
   :type data: :class:`Data`
   :param xfactor: Downsampling factor in time direction.
   :type xfactor: integer
   :param yfactor: Downsampling factor in frequency direction.
   :type yfactor: integer
   :param masked: ``true`` means take flags into account during averaging
   :type masked: boolean
   :return: Downsampled version of input data.
   :rtype: :class:`Data`
   
.. function:: high_pass_filter(data, xsize, ysize, xsigma, ysigma)

   Apply a Gaussian high-pass filter to the data. This removes the
   diffuse 'background' in the data. With appropriate settings, it
   can filter the signal of interest (slow sinusoidal signals), making
   the interference easier to detect.
   
   The function convolves the data with a 2D "1 minus Gaussian" kernel.
   The kernel is clipped at the edges. The sigma parameters
   define the strength (band-limit) of the filter: lower values remove
   more of the diffuse structure.

   :param data: The data (modified in place).
   :type data: :class:`Data`
   :param xsize: Kernel size in time direction
   :type xsize: integer
   :param ysize: Kernel size in frequency direction
   :type ysize: integer
   :param xsigma: Gaussian width in time direction.
   :type xsigma: number
   :param ysigma: Gaussian width in frequency direction.
   :type ysigma: number
   
.. function:: low_pass_filter(data, xsize, ysize, xsigma, ysigma)

   Apply a Gaussian low-pass filter to the data. It convolves the
   data with a Gaussian. See :meth:`high_pass_filter` for further details.
   
   :param data: The data (modified in place).
   :type data: :class:`Data`
   :param xsize: Kernel size in time direction
   :type xsize: integer
   :param ysize: Kernel size in frequency direction
   :type ysize: integer
   :param xsigma: Gaussian width in time direction.
   :type xsigma: number
   :param ysigma: Gaussian width in frequency direction.
   :type ysigma: number

.. function:: norm(input_data)

   Creates a new :class:`Data` object for which the data values have been replaced by
   their norm. If the data is complex, a complex norm is performed. In all other
   cases, a real-valued norm is performed (i.e., the data is squared).
   
   :param input_data: Data object (not modified)
   :type input_data: :class:`Data`
   :returns: Norm of input_data, element-wise applied.
   :rtype: :class:`Data`
   
.. function:: normalize_bandpass(data)

   Normalizes the RMS over frequency. If multiple polarizations are present in the data,
   the RMS over the combination of all polarizations is calculated and normalized.

   Available since :doc:`v3.1 <changelogs/v3.01>`.
   
   :param data: The data (modified in place).
   :type data: :class:`Data`

.. function:: normalize_subbands(data, nr_subbands)

   Remove jumps between subbands. A subband is
   in this context a number of adjacent channels, equally spaced over
   the bandwidth. This function therefore assumes that all subbands
   have an equal number of channels. 
   
   Each subband is scaled such that the standard deviation of the visibilities
   in a subband is unity. To avoid influence from interference, a stable method
   is used to estimate the standard deviation (Winsorized standard deviation).
   
   A typical use-case for this function is the MWA phase 1 and 2. The 30 MHz
   bandwidth of the MWA is split in 24 'course channels', each consisting
   of 128 channels. Each course channel has an independent gain, and needs
   normalization before it can be compared with adjacent course channels.

   :param data: The data (modified in place).
   :type data: :class:`Data`
   :param nr_subbands: Number of subbands.
   :type nr_subbands: integer

.. function:: print_polarization_statistics(data)
   :deprecated:
   
   Print RFI percentages per polarization to the command line.
   
   :param data: Input data.
   :type data: :class:`Data`
  
.. function:: require_max_version(version)

   Checks if the aoflagger version is lower or equal to the provided version.
   If the condition is not met, an error is thrown. This function can be used
   when it is known a strategy is making use of Lua functionality that was
   changed in newer aoflagger versions.
   
   The version string can be of the form "major", "major.min" or
   "major.minor.subminor". The version is only checked up to the level that
   is specified: requiring at most version "``3.2``" will allow version
   "``3.2.1``", but not version "``3.3.0``" or "``4.0``". To disallow version
   "``3.2.1``", a maximum version of "``3.2.0``" should be specified.
   
   Available since :doc:`v3.1 <changelogs/v3.01>`.

   See also :meth:`require_min_version`.
   
   :param version: Latest version that is allowed, e.g. ``"3.0.4"``.
   :type version: string
   
.. function:: require_min_version(version)

   Checks if the aoflagger version is equal to or newer than the provided version
   string. If the condition is not met, an error is thrown. 
   This is a useful way of notifying users that their version of aoflagger
   is too old. A version of aoflagger should (only) be considered too old when
   the strategy requires a function, method or other functionality that is not
   available in versions before the specified version.
   
   The version string can be of the form "major", "major.min" or
   "major.minor.subminor". The version is only checked up to the level that
   is specified: requiring version "``3.2``" will allow versions such as
   "``3.2-alpha``" and "``3.2.1``".

   Available since :doc:`v3.1 <changelogs/v3.01>`.
   
   :param version: Minimum version that is allowed, e.g. ``"3.0.4"``.
   :type version: string
   
.. function:: save_heat_map(filename, data)

   Save the data as a "heat map" image. The type is determined from the
   extension. Supported extensions are ``.svg``, ``.png`` and ``.pdf``.
   
   :param filename: Path to image to be written.
   :type filename: string
   :param data: Input data.
   :type data: :class:`Data`

.. function:: scale_invariant_rank_operator(data, xlevel, ylevel)

   Extend flags in time and frequency direction in a scale-invariant manner.
   This fills holes in the flag mask and makes flag sequences longer.
   Details are described in
   `Offringa et al. 2012 <https://arxiv.org/abs/1201.3364>`_.
   
   :param data: The data (modified in place).
   :type data: :class:`Data`
   :param xlevel: aggressiveness in time-direction
   :type xlevel: number
   :param ylevel: aggressiveness in frequency-direction
   :type ylevel: number

.. function:: scale_invariant_rank_operator_masked(data, mask_data, xlevel, ylevel, penalty)

   Perform the same operation as :meth:`scale_invariant_rank_operator`, but
   with an input mask that identifies invalid data.
   Invalid data is treated differently, and the penalty parameter selects
   how it is treated. With a penalty of 0, it is as if invalid samples are
   removed before applying the operator. With a penalty of 1, invalid samples
   are counted in the same way as unflagged samples (i.e., they penalize
   their extension). A typical penalty value is 0.1.
   For backwards compatibility, penalty may be left out, in
   which case a value of 0.1 is used.

   Available since :doc:`v3.1 <changelogs/v3.01>`. The penalty parameter is
   available since :doc:`v3.2 <changelogs/v3.02>`.
   
   :param data: The data (modified in place).
   :type data: :class:`Data`
   :param mask_data: The data that is used as mask.
   :type mask_data: :class:`Data`
   :param xlevel: aggressiveness in time-direction
   :type xlevel: number
   :param ylevel: aggressiveness in frequency-direction
   :type ylevel: number
   :param penalty: penalty given to the extension through
      invalid regions.
   :type penalty: number

.. function:: set_progress(progress, max_progress)

   Notify user of the progress of this call. The gui uses this
   information to show a progress bar to the user.
   Example: when the :meth:`execute` function iterates over the
   polarizations, progress can be reported by calling
   ``aoflagger.set_progress(curpol, npol)`` inside the loop.

   :param progress: current progress
   :type progress: integer
   :param max_progress: value of progress when complete
   :type max_progress: integer

.. function:: set_progress_text(task_description)

   Notify user of the current task being done. The description can be anything,
   and can literally be presented to the user.

   :param task_description: Description string.
   :type task_description: string

.. function:: sqrt(input_data)

   Creates a new :class:`Data` object where all data values are replaced by their
   square root. If the data is complex, a complex square root is performed. In all other
   cases, a real-valued square root is performed.
   
   :param input_data: Data object (not modified)
   :type input_data: :class:`Data`
   :returns: Square root of input_data, element-wise applied.
   :rtype: :class:`Data`
   
.. function:: sumthreshold(data, x_threshold_factor, y_threshold_factor, x_direction, y_direction)

   Run the SumThreshold algorithm on the data. This algorithm detects sharp,
   line-shaped features in the time-frequency domain that are typical for RFI.
   See `Offringa et al. (2010) <https://arxiv.org/abs/1002.1957>`_ for details
   about the algorithm.
   
   The thresholds are relative to a (stable) estimate of the noise in the
   visibilities. They define the base sensitivity of the algorithm.
   Lower values will detect more features. A reasonable value for the
   thresholds is 1.
   
   The ``x_direction``/``y_direction`` parameters turn
   detection in their particular directions on and off. If a direction is turned off, the
   threshold factor for that direction is ignored. Note that detection in
   *x*-direction (which is the time-direction) means detection of contiguous high-power samples
   in time, such as transmitters that occupy the same channel continuously.
   The *y*-direction detection is sensitive to transient, broadband RFI.

   :param data: The data (modified in place)
   :type data: :class:`Data`
   :param x_threshold_factor: Threshold factor in time direction
   :type x_threshold_factor: number
   :param y_threshold_factor: Threshold factor in frequency direction
   :type y_threshold_factor: number
   :param x_direction: Enable flagging in time direction
   :type x_direction: boolean
   :param y_direction: Enable flagging in frequency direction
   :type y_direction: boolean
   
.. function:: sumthreshold_masked(data, mask_data, x_threshold_factor, y_threshold_factor, x_direction, y_direction)

   Same as :meth:`sumthreshold`, but with a mask. Visibilities that are flagged
   in the mask are considered to be visibilities that have not been sampled and
   are removed from the SumThreshold operation. A typical case
   for this is to make sure that correlator faults, shadowing and
   band-edges are correctly treated.

   :param data: The data (modified in place).
   :type data: :class:`Data`
   :param mask_data: The data that is used as mask
   :type mask_data: :class:`Data`
   :param x_threshold_factor: Threshold factor in time direction
   :type x_threshold_factor: number
   :param y_threshold_factor: Threshold factor in frequency direction
   :type y_threshold_factor: number
   :param x_direction: Enable flagging in time direction
   :type x_direction: boolean
   :param y_direction: Enable flagging in frequency direction
   :type y_direction: boolean
   
.. function:: threshold_channel_rms(data, threshold, flag_low_outliers)

   Calculate the root-mean-square (RMS) for each channel and flags channels
   that have an outlier RMS. The threshold is a "sigma level". Typical values
   for the threshold are therefore around 3.
   
   :param data: The data (modified in place).
   :type data: :class:`Data`
   :param threshold: Sigma-level of threshold.
   :type threshold: number
   :param flag_low_outliers: Flag channels with low RMS.
   :type flag_low_outliers: boolean

.. function:: threshold_timestep_rms(data, threshold)

   Like :meth:`threshold_channel_rms`, but thresholds *timesteps* with outlier
   RMS. Both timesteps with high and low RMS values are flagged.

   :param data: The data (modified in place).
   :type data: :class:`Data`
   :param threshold: Sigma-level of threshold.
   :type threshold: number

.. function:: trim_channels(data, start_channel, end_channel)

   Create a new data object from a subset of the input data. This can
   be used to flag a subset of the data, together with :meth:`copy_to_channel`
   to copy the result back.
   All channels for which 
   ``start_channel`` <= channel index < ``end_channel`` are
   copied into the result.  All timesteps are copied.
   
   Available since :doc:`v3.1 <changelogs/v3.01>`.
   
   :param data: Input data (unchanged).
   :type data: :class:`Data`
   :param start_channel: Index of first channel
   :type start_channel: integer
   :param end_channel: Index of end of the channel range. The end range
      is excluding.
   :type end_channel: integer
   :return: A new data object, trimmed as specified.
   :rtype: :class:`Data`
  
.. function:: trim_frequencies(data, start_frequency, end_frequency)

   This function is equal to :meth:`trim_channels`, except that the
   channel range is specified with frequency values.
   All channels for which 
   ``start_frequency`` <= channel frequency < ``end_frequency`` are
   copied into the result. All timesteps are copied.
   
   :meth:`copy_to_frequency` can be used to copy the result back
   after processing.

   Available since :doc:`v3.1 <changelogs/v3.01>`.
   
   :param data: Input data (unchanged).
   :type data: :class:`Data`
   :param start_frequency: Start frequency in MHz of the selected range. 
   :type start_frequency: number
   :param end_frequency: End frequency in MHz of the channel range.
   :type end_frequency: number
   :return: A new data object, trimmed as specified.
   :rtype: :class:`Data`

.. function:: upsample_image(input_data, destination_data, xfactor, yfactor)

   Increase the resolution of the data. This function is to restore the
   resolution of the data after having called :meth:`downsample`.
   ``input_data`` is normally the data that was returned by :meth:`downsample`,
   and ``destination_data`` is the input object that was specified as parameter.
   The upsampling is done by nearest neighbour interpolation.
   
   The x and y factors should be the equal to the values specified in the call
   to `downsample`. The size of the ``destination_data`` is not changed: the
   input data is stretched by the given factors, and trimmed to the destination
   size in case the image dimensions were not exactly divisable by the factors.
   
   The function only upsamples the visibilities, not the flags. To upsample the
   flags, see :meth:`upsample_mask`.

   :param input_data: Input low-resolution data (not modified).
   :type input_data: :class:`Data`
   :param destination_data: Where the result will be stored.
   :type destination_data: :class:`Data`
   :param xfactor: Upsampling factor in time direction.
   :type xfactor: integer
   :param yfactor: Upsampling factor in frequency direction.
   :type yfactor: integer
   
.. function:: upsample_mask(input_data, destination_data, xfactor, yfactor)

   Increase the resolution of the mask. It is identical to :meth:`upsample_image`,
   but works with the mask (flags) instead of the image (visibilities).

   :param input_data: Input low-resolution data (not modified).
   :type input_data: :class:`Data`
   :param destination_data: Where the result will be stored.
   :type destination_data: :class:`Data`
   :param xfactor: Upsampling factor in time direction.
   :type xfactor: integer
   :param yfactor: Upsampling factor in frequency direction.
   :type yfactor: integer
   
.. function:: visualize(data, label, sorting_index)

   Save a visualization of the data for inspection in ``rfigui``. When this
   strategy runs outside of the ``rfigui``, the call is ignored. Can be used
   to e.g. inspect partial results.
   
   :param data: Input data (not modified).
   :type data: :class:`Data`
   :param label: A short description that is displayed to the user.
   :type label: string
   :param sorting_index: Where to place this visualization in the list
       of visualization
   
