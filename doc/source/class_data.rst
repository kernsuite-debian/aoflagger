Data class
==========

.. default-domain:: lua

.. class:: Data

    The Data class contains the visibility data, flag masks and meta data.
    
    It may hold information for multiple polarizations, channels and timesteps,
    such that it contains the data for a particular observing field,
    spectral window (band) and antenna-pair (or single dish).
    The data objects thus holds a dynamic spectrum for each polarization.
    
    The visibility data can consist of complex values, or single-float
    values representing the real, imaginary, amplitude or phase of the
    visibilities (see :meth:`get_complex_state`).
    
    The Data class is a Lua ``userdata`` object.
    Assigning one Data variable to another will therefore cause both variables
    to point to the same object, and changing one will also change the other.
    To create an independent object, :meth:`copy` can be used.
    
    Internally, AOFlagger uses a copy-on-write-like mechanism for the
    visibility sets, flag masks and meta data. It is not required for users
    of the class to know about these implementation details, but the
    relevant thing to know is that copy or assignment operations are generally
    fast and don't increase memory usage.

Method summary
^^^^^^^^^^^^^^

* Copy & Modification
    - :meth:`~Data.clear_mask`
    - :meth:`~Data.copy`
    - :meth:`~Data.flag_nans`
    - :meth:`~Data.flag_zeros`
    - :meth:`~Data.join_mask`
    - :meth:`~Data.set_mask`
    - :meth:`~Data.set_mask_for_channel_range`
    - :meth:`~Data.set_polarization_data`
    - :meth:`~Data.set_visibilities`
    - :meth:`~Data.__sub`
* Conversion
    - :meth:`~Data.convert_to_complex`
    - :meth:`~Data.convert_to_polarization`
* Meta-data
    - :meth:`~Data.get_antenna1_index`
    - :meth:`~Data.get_antenna1_name`
    - :meth:`~Data.get_antenna2_index`
    - :meth:`~Data.get_antenna2_name`
    - :meth:`~Data.get_baseline_angle`
    - :meth:`~Data.get_baseline_distance`
    - :meth:`~Data.get_baseline_vector`
    - :meth:`~Data.get_complex_state`
    - :meth:`~Data.get_frequencies`
    - :meth:`~Data.get_polarizations`
    - :meth:`~Data.get_times`
    - :meth:`~Data.has_metadata`
    - :meth:`~Data.is_auto_correlation`
    - :meth:`~Data.is_complex`


Detailed descriptions
^^^^^^^^^^^^^^^^^^^^^

    .. method:: Data.clear_mask(data)
    
        Clear the flag mask. Unflags all visibilities (sets all flags to
        false).
        
        :param data: Data for which the mask is cleared. 
        :type data: :class:`Data`
    
    .. method:: Data.convert_to_complex(data, new_state)
    
        Make a new :class:`Data` object with a different complex-value state.
        Complex input data (data with :meth:`get_complex_state` ==
        ``"complex"``) can be converted to real, imaginary, amplitude or
        phase values.
        Amplitude data (:meth:`get_complex_state` == ``"amplitude"``)
        can also be converted back to complex.
        In that case the phases become zero. Other conversions are not
        implemented and will cause an error.
        
        The complex state of an :class:`Data` object is stored internally and
        can be acquired by calling :meth:`get_complex_state`.
        
        :param data: Input data (unchanged).
        :type data: :class:`Data`
        :param new_state: ``"complex"``, ``"real"``, ``"imaginary"``,
            ``"amplitude"`` or ``"phase"``.
        :type new_state: string
        :return: New object with :meth:`get_complex_state` == ``new_state``
        :rtype: :class:`Data`
       
    .. method:: Data.convert_to_polarization(data, new_polarization)
    
        Make a new :class:`Data` object by converting the polarization.
        If the input data does not hold the polarimetric data to convert
        to the requested polarization, an error is thrown. For example,
        converting to ``"i"`` from data for which
        :meth:`get_polarizations` == ``{"xx","yy"}`` is possible, but
        converting to ``"q"`` from data with
        :meth:`get_polarizations` == ``{"ll"}`` is not.
        
        This method can also be used to extract a single polarization
        from the set of available polarizations, e.g.
        
        .. code-block:: lua
        
            xxdata = data:convert_to_polarization("xx")
            
        for data with :meth:`get_polarizations`
        == ``{"xx", "xy", "yx", "yy"}``.
        
        :param data: input data (unchanged).
        :type data: :class:`Data`
        :param new_polarization: ``"i"``, ``"q"``, ``"u"``, ``"v"``,
            ``"xx"``, ``"xy"``, ``"yx"``, ``"yy"``, ``"rr"``, ``"rl"``,
            ``"lr"`` or ``"ll"``.
        :type new_polarization: string
        
    .. method:: Data.copy(data)
    
        Make a value copy of the data.
        
        :param data: Source data.
        :type data: :class:`Data`
        :return: Value copy of input data.
        :rtype: :class:`Data`
    
    .. method:: Data.flag_nans(data)
    
        Flag visibilities that are 'not a number' (nan) or hold overflow.
        Each polarization is independently searched for nans, and its
        mask is updated for that polarization (this is different from
        :meth:`Data.flag_zeros`).
        
        :since: AOFlagger 3.1.
        
    .. method:: Data.flag_zeros(data)
    
        Flag visibilities that are exactly zero. This corrects for
        correlators that output zeros during faults, such as network problems.
        
        It flags samples when the sum of visibilities over polarizations is
        zero. When it is necessary to flag the polarizations independently, the
        statement should be placed inside a loop, e.g.:
        
        .. code-block:: lua
        
            for _,polarization in ipairs(data.get_polarizations()) do
              pol_data = data:convert_to_polarization(polarization)
              flag_zeros(pol_data)
              data:set_polarization_data(polarization, pol_data)
            end
    
        :param data: Data (modified inplace).
        :type data: :class:`Data`
    
    .. method:: Data.get_antenna1_index(data)
    
        Get first antenna index of the two correlated antennas.
        Throws an error of the antenna metadata is not available
        (see :meth:`has_metadata`).

        :param data: Input data (unchanged).
        :type data: :class:`Data`
        :return: Index of first antenna
        :rtype: integer
    
    .. method:: Data.get_antenna1_name(data)
    
        Get name of first antenna of the two correlated antennas.
        Throws an error of the antenna metadata is not available
        (see :meth:`has_metadata`).

        :param data: Input data (unchanged).
        :type data: :class:`Data`
        :return: Name of first antenna
        :rtype: string
    
    .. method:: Data.get_antenna2_index(data)
    
        Get second antenna index of the two correlated antennas.
        Throws an error of the antenna metadata is not available
        (see :meth:`has_metadata`).

        :param data: Input data (unchanged).
        :type data: :class:`Data`
        :return: Index of second antenna
        :rtype: integer
        
    .. method:: Data.get_antenna2_name(data)
    
        Get name of second antenna of the two correlated antennas.
        Throws an error of the antenna metadata is not available
        (see :meth:`has_metadata`).

        :param data: Input data (unchanged).
        :type data: :class:`Data`
        :return: Name of second antenna
        :rtype: string
    
    .. method:: Data.get_baseline_angle(data)
    
        Get angle of this baseline. This is that angle between
        the line from antenna2 to antenna1 and North.
        Throws an error of the antenna metadata is not available
        (see :meth:`has_metadata`).
        
        :param data: Input data (unchanged)
        :type data: :class:`Data`
        :return: Baseline angle in radians
        :rtype: number
    
    .. method:: Data.get_baseline_distance(data)
    
        Get distance of the antenna1-antenna2 baseline in meters.
        Throws an error of the antenna metadata is not available
        (see :meth:`has_metadata`).
        
        :param data: Input data (unchanged).
        :type data: :class:`Data`
        :return: Baseline distance in meters
        :rtype: number
    
    .. method:: Data.get_baseline_vector(data)
    
        Get a table with items ``x``, ``y`` and ``z`` that form the three-
        dimensional vector between antennas 1 and 2.
        Throws an error of the antenna metadata is not available
        (see :meth:`has_metadata`).
        
        :param data: Input data (unchanged).
        :type data: :class:`Data`
        :return: Baseline vector in meters
        :rtype: table
    
    .. method:: Data.get_complex_state(data)
    
        Get the state that the visibilities represent. This can be ``"phase"``,
        ``"amplitude"``, ``"real"``, ``"imaginary"`` or ``"complex"``. When the
        data is complex, each visibility consists of two number. Conversions
        can be performed with :meth:`convert_to_complex`.
        
        :param data: Input data (unchanged).
        :type data: :class:`Data`
        :return: Complex state of data.
        :rtype: string
    
    .. method:: Data.get_frequencies(data)
    
        Get the frequencies of the channels.
        Throws an error of the spectral window metadata is not available
        (see :meth:`has_metadata`).

        :param data: Input data (unchanged).
        :type data: :class:`Data`
        :return: List that maps channel nr to frequency in Hz.
        :rtype: table
    
    .. method:: Data.get_polarizations(data)
    
        Get the list of polarizations provided by the data.
        See :meth:`convert_to_polarization` for the list of possible
        polarization names.
        
        :param data: Input data (unchanged).
        :type data: :class:`Data`
        :return: List that maps polarization nr to a string.
        :rtype: table
    
    .. method:: Data.get_times(data)
    
        Get the time of each timestep in these data.
        Throws an error of the time metadata is not available
        (see :meth:`has_metadata`).

        :param data: Input data (unchanged).
        :type data: :class:`Data`
        :return: List that maps timestep nr to MJD time in s.
        :rtype: table
    
    .. method:: Data.has_metadata(data)
    
        Returns whether metadata is completely present.
        Not all data formats (or simulations) provide all metadata
        items and some of the other methods (e.g. :meth:`get_times`) may throw
        an error because of this. If this function returns ``true``, all these
        functions will succeed.

        :param data: Input data (unchanged).
        :type data: :class:`Data`
        :return: ``true`` in case all metadata is available, ``false`` otherwise.
        :rtype: boolean
    
    .. method:: Data.is_auto_correlation(data)
    
        Determine whether this baseline is an auto-correlation.
        This is the case if :meth:`get_antenna1_index` ==
        :meth:`get_antenna2_index`. Unlike the ``get_antenna*``
        functions, this method won't throw an error when no meta-data is
        available. ``false`` is returned in that case.
        
        :param data: Input data (unchanged).
        :type data: :class:`Data`
        :return: ``true`` when this is an auto-correlation.
        :type: boolean
    
    .. method:: Data.is_complex(data)
    
        :param data: Input data (unchanged).
        :type data: :class:`Data`
        :return: ``true`` when :meth:`get_complex_state` == ``"complex"``
        :type: boolean
    
    .. method:: Data.join_mask(first_data, second_data)
    
        Join two masks together. A flag will be set when it is set in either or
        both of the input data sets.
        
        :param first_data: First mask and destination of operation.
        :type first_data: :class:`Data`
        :param second_data: Second mask (unchanged).
        :type second_data: :class:`Data`
    
    .. method:: Data.set_mask(destination_data, mask_data)
    
        Assign the mask of one :class:`Data` object to another.

        :param first_data: Destination data (changed inplace).
        :type first_data: :class:`Data`
        :param second_data: Source data (unchanged).
        :type second_data: :class:`Data`
    
    .. method:: Data.set_mask_for_channel_range(destination_data, mask_data, freq_start, freq_end)
    
        Partially assign the mask of one :class:`Data` object to another.
        The flag mask of channels within the given frequency range are copied
        from ``mask_data`` to ``destination_data``.
        
        This can for example be useful when a certain channel range should not
        be flagged, by partially copying the initial flags to the mask
        produced by the RFI detection.
        
        :param first_data: Destination data (changed inplace).
        :type first_data: :class:`Data`
        :param second_data: Source data (unchanged).
        :type second_data: :class:`Data`
        :param freq_start: Frequency range start in MHz
        :type freq_start: number
        :param freq_end: Frequency range end in MHz
        :type freq_end: number
    
    .. method:: Data.set_polarization_data(destination_data, polarization, source_data)
    
        Replace one polarization of a :class:`Data` object with some other data.
        The ``source_data`` should have only one polarization. The typical
        use-case for this method is to loop over polarizations and modify
        them one by one, e.g.:
        
        .. code-block:: lua
        
            for _,polarization in ipairs(data:get_polarizations()) do 
              pol_data = input:convert_to_polarization(polarization)
              -- Change pol_data here...
              data:set_polarization_data(polarization, pol_data)
            end
            
        Note that the two data sets should have the same complex state
        (see :meth:`get_complex_state()`). This method copies both the
        mask and the visibilities. The meta-data is unchanged.
    
        :param destination_data: Destination data (changed inplace).
        :type destination_data: :class:`Data`
        :param polarization: Name of polarization to change
        :type polarization: string 
        :param source_data: Source data (unchanged).
        :type source_data: :class:`Data`
       
    .. method:: Data.set_visibilities(destination_data, visibility_data)
    
        Assign the visibility data from one :class:`Data` object to another.
        The flagmask and meta-data are unchanged. The two sets should have
        the same number of polarizations and the same complex state.
        
        :param destination_data: Destination data (changed inplace).
        :type destination_data: :class:`Data`
        :param visibility_data: Source data (unmodified).
        :type visibility_data: :class:`Data`
    
    .. method:: Data.__gc(data)
    
        Internal method for garbage collection function of the Data class.
        This will free the allocated data when those data are no longer 
        used by other Data objects. Note that AOFlagger will immediately
        clear data when the :meth:`execute` function is finished, even when
        the class hasn't been garbage collected yet. 
        
        TODO add a method ``set_persistent`` to disable this.
    
        :param destination_data: Garbage collected data.
        :type destination_data: :class:`Data`
        
    .. method:: Data.__sub(lhs_data, rhs_data)
    
        Lua-special method that makes it possible to subtract :class:`Data`
        objects, e.g.:
        
        .. code-block:: lua
 
            data = data - filtered_data
            
        The output data will have the same meta-data and masks as ``lhs_data``.
        
            
        :param lhs_data: Left-hand side data (not modified)
        :type lhs_data: :class:`Data`
        :param rhs_data: Right-hand side data (not modified)
        :type rhs_data: :class:`Data`
        :returns: Left - right
        :rtype: :class:`Data`

