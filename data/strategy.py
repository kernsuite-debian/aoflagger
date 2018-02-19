import aoflagger
import copy
import numpy

def flag(input):

  # Values below can be tweaked
  flag_polarizations = input.polarizations()
  flag_representations = [ aoflagger.ComplexRepresentation.AmplitudePart ]

  iteration_count = 3
  threshold_factor_step = 2.0
  base_threshold = 1.4
  
  # Use above values to calculate thresholds in iteration
  r = range((iteration_count-1), 0, -1)
  threshold_factors = numpy.power(threshold_factor_step, r)

  inpPolarizations = input.polarizations()
  input.clear_mask()
  
  for polarization in flag_polarizations:
      
    data = input.convert_to_polarization(polarization)

    for representation in flag_representations:

      data = data.convert_to_complex(representation)
      original_image = copy.copy(data)
      
      for threshold_factor in threshold_factors:
      
        print 'Flagging polarization ' + str(polarization) + ' (' + str(representation) + ', ' + str(threshold_factor) + ')'

        aoflagger.sumthreshold(data, threshold_factor * base_threshold, True, True)
        chdata = copy.copy(data)
        aoflagger.threshold_timestep_rms(data, 3.5)
        aoflagger.threshold_channel_rms(chdata, 3.0 * threshold_factor, True)
        data.join_mask(chdata)
        
        data.set_image(original_image)
        resized_data = aoflagger.shrink(data, 3, 3)
        aoflagger.low_pass_filter(resized_data, 21, 31, 1.6, 2.2)
        aoflagger.enlarge(resized_data, data, 3, 3)
        data = original_image - data

      aoflagger.sumthreshold(data, base_threshold, True, True)

    if polarization in inpPolarizations:
      data = data.make_complex()
      input.set_polarization_data(polarization, data)
    else:
      input.join_mask(polarization, data)
      
  aoflagger.scale_invariant_rank_operator(input, 0.2, 0.2)
  aoflagger.threshold_timestep_rms(input, 4.0)
  
def test_sumthreshold(input):
  # Values below can be tweaked
  flag_polarizations = input.polarizations()
  flag_representations = [ aoflagger.ComplexRepresentation.AmplitudePart ]

  iteration_count = 3
  threshold_factor_step = 2.0
  base_threshold = 1.4
  
  # Use above values to calculate thresholds in iteration
  r = range((iteration_count-1), 0, -1)
  threshold_factors = numpy.power(threshold_factor_step, r)

  inpPolarizations = input.polarizations()
  input.clear_mask()
  
  for polarization in flag_polarizations:
      
    data = input.convert_to_polarization(polarization)
    for representation in flag_representations:

      data = data.convert_to_complex(representation)
      aoflagger.sumthreshold(data, base_threshold, True, True)

    if polarization in inpPolarizations:
      data = data.make_complex()
      input.set_polarization_data(polarization, data)
    else:
      input.join_mask(polarization, data)
      
aoflagger.set_flag_function(flag)
                                          
print 'strategy.py parsed'
