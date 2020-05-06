
function contains(arr, val)
  for _,v in ipairs(arr) do
    if v == val then return true end
  end
end

function options()
  t = { }
  t.baselines = "all"
  return t
end

function execute (input)
  -- Values below can be tweaked
  flag_polarizations = input:polarizations()

  -- Options are: phase, amplitude, real, imaginary, complex
  flag_representations = { "amplitude" }

  iteration_count = 3
  threshold_factor_step = 2.0
  base_threshold = 1.4
  
  -- Calculate thresholds in each iteration

  inpPolarizations = input:polarizations()
  input:clear_mask()
  
  for _,polarization in ipairs(flag_polarizations) do
 
    data = input:convert_to_polarization(polarization)

    for _,representation in ipairs(flag_representations) do

      data = data:convert_to_complex(representation)
      original_image = data:copy()

      for i=1,iteration_count-1 do
        threshold_factor = math.pow(threshold_factor_step, iteration_count-i)

        sumthr_level = threshold_factor * base_threshold
        aoflagger.sumthreshold(data, sumthr_level, sumthr_level, true, true, false)

        -- Do timestep & channel flagging
        chdata = data:copy()
        aoflagger.threshold_timestep_rms(data, 3.5)
        aoflagger.threshold_channel_rms(chdata, 3.0 * threshold_factor, true)
        data:join_mask(chdata)

        -- High pass filtering steps
        data:set_visibilities(original_image)
        resized_data = aoflagger.downsample(data, 3, 3, false)
        aoflagger.low_pass_filter(resized_data, 21, 31, 1.6, 2.2)
        aoflagger.upsample(resized_data, data, 3, 3)

        -- In case this script is run from inside rfigui, calling
        -- the following visualize method will add the current result
        -- to the list of displayable visualizations.
        -- If the script is not running inside rfigui, the call is ignored.
        aoflagger.visualize(data, "Fit #"..i, i-1)

        tmp = original_image - data
        tmp:set_mask(data)
        data = tmp

        aoflagger.visualize(data, "Residual #"..i, i+iteration_count)

      end -- end of iterations

      aoflagger.sumthreshold(data, base_threshold, base_threshold, true, true, false)
    end -- end of complex representation iteration

    if contains(inpPolarizations, polarization) then
      data = data:make_complex()
      input:set_polarization_data(polarization, data)
    else
      input:join_mask(polarization, data)
    end
  end -- end of polarization iterations

  aoflagger.scale_invariant_rank_operator(input, 0.2, 0.2)
  aoflagger.threshold_timestep_rms(input, 4.0)

  print("Flagged baseline "..input:get_antenna1_name().." x "..input:get_antenna2_name())

end

