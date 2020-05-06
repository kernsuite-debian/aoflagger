
function contains(arr, val)
  for _,v in ipairs(arr) do
    if v == val then return true end
  end
end

function dump(o)
   if type(o) == 'table' then
      local s = '{ '
      for k,v in pairs(o) do
         if type(k) ~= 'number' then k = '"'..k..'"' end
         s = s .. '['..k..'] = ' .. dump(v) .. ','
      end
      return s .. '} '
   else
      return tostring(o)
   end
end

function execute (input)

  -- Values below can be tweaked
  flag_polarizations = input:polarizations()

  -- Options are: phase, amplitude, real, imaginary, complex
  flag_representations = { "amplitude" }

  base_threshold = 1.2
  iteration_count = 3
  threshold_factor_step = 2.0
  exclude_original_flags = true
  
  inpPolarizations = input:polarizations()

  --aoflagger.apply_bandpass(input, "bandpassfile.txt")

  if(exclude_original_flags) then
    copy_of_input = input:copy()
  else
    input:clear_mask()
  end
  
  for _,polarization in ipairs(flag_polarizations) do
 
    data = input:convert_to_polarization(polarization)

    for _,representation in ipairs(flag_representations) do

      data = data:convert_to_complex(representation)
      original_data = data:copy()

      for i=1,iteration_count-1 do
        threshold_factor = math.pow(threshold_factor_step, iteration_count-i)

        sumthr_level = threshold_factor * base_threshold
        if(exclude_original_flags) then
          aoflagger.sumthreshold_with_missing(data, original_data, sumthr_level, sumthr_level, true, true)
        else
          aoflagger.sumthreshold(data, sumthr_level, sumthr_level, true, true)
        end

        -- Do timestep & channel flagging
        chdata = data:copy()
        aoflagger.threshold_timestep_rms(data, 3.5)
        aoflagger.threshold_channel_rms(chdata, 3.0 * threshold_factor, true)
        data:join_mask(chdata)

        -- High pass filtering steps
        data:set_visibilities(original_data)
        if(exclude_original_flags) then
          data:join_mask(original_data)
        end
        data:flag_zeros()
        resized_data = aoflagger.downsample(data, 1, 175, true)
        aoflagger.low_pass_filter(resized_data, 21, 31, 2.5, 5.0)
        aoflagger.upsample(resized_data, data, 1, 175)

        -- In case this script is run from inside rfigui, calling
        -- the following visualize function will add the current result
        -- to the list of displayable visualizations.
        -- If the script is not running inside rfigui, the call is ignored.
        aoflagger.visualize(data, "Fit #"..i, i-1)

        tmp = original_data - data
        tmp:set_mask(data)
        data = tmp

        aoflagger.visualize(data, "Residual #"..i, i+iteration_count)

      end -- end of iterations

      if(exclude_original_flags) then
        aoflagger.sumthreshold_with_missing(data, original_data, base_threshold, base_threshold, true, true)
      else
        aoflagger.sumthreshold(data, base_threshold, base_threshold, true, true)
      end
    end -- end of complex representation iteration

    if(exclude_original_flags) then
      data:join_mask(original_data)
    end
    aoflagger.threshold_timestep_rms(data, 4.0)

    if contains(inpPolarizations, polarization) then
      data = data:make_complex()
      input:set_polarization_data(polarization, data)
    else
      input:join_mask(polarization, data)
    end

    aoflagger.visualize(data, "Residual #"..iteration_count, 2*iteration_count)
  end -- end of polarization iterations

  if(exclude_original_flags) then
    aoflagger.scale_invariant_rank_operator_with_missing(input, copy_of_input, 0.2, 0.2)
  else
    aoflagger.scale_invariant_rank_operator(input, 0.2, 0.2)
  end

  -- The following statement restores the flagging as it was before RFI detection
  -- for the frequency range 1418-1424 MHz. This range is considered clean for Apertif,
  -- hence any flags there are more likely to be false positives. This is in
  -- particular important when observing bright Galaxies that might show HI lines that
  -- are bright enough to get flagged.
  input:set_mask_for_channel_range(copy_of_input, 1418, 1424)

  input:flag_zeros()

end

