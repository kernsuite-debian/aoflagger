#include "timefrequencydata.h"
#include "stokesimager.h"

#include "../util/ffttools.h"

namespace {
/// Performs complex division and updates lhs
void DivideComplexSinglePolarization(TimeFrequencyData& lhs,
                                     const TimeFrequencyData& rhs) {
  const size_t width = lhs.ImageWidth();
  const size_t height = lhs.ImageHeight();
  const Image2DCPtr& l_real = lhs.GetImage(0);
  const Image2DCPtr& l_imag = lhs.GetImage(1);
  const Image2DCPtr& r_real = rhs.GetImage(0);
  const Image2DCPtr& r_imag = rhs.GetImage(1);
  Image2DPtr real_result = Image2D::CreateUnsetImagePtr(width, height);
  Image2DPtr imag_result = Image2D::CreateUnsetImagePtr(width, height);
  for (size_t y = 0; y != height; ++y) {
    for (size_t x = 0; x != width; ++x) {
      const std::complex<num_t> lhs_value(l_real->Value(x, y),
                                          l_imag->Value(x, y));
      const std::complex<num_t> rhs_value(r_real->Value(x, y),
                                          r_imag->Value(x, y));
      const std::complex<num_t> result_value = lhs_value / rhs_value;
      real_result->SetValue(x, y, result_value.real());
      imag_result->SetValue(x, y, result_value.imag());
    }
  }
  lhs.SetImage(0, std::move(real_result));
  lhs.SetImage(1, std::move(imag_result));
}

/// Performs real division and updates lhs
void DivideRealSinglePolarization(TimeFrequencyData& lhs,
                                  const TimeFrequencyData& rhs) {
  const size_t width = lhs.ImageWidth();
  const size_t height = lhs.ImageHeight();
  Image2DCPtr l_data = lhs.GetImage(0);
  Image2DCPtr r_data = rhs.GetImage(0);
  Image2DPtr result_image = Image2D::CreateUnsetImagePtr(width, height);
  for (size_t y = 0; y != height; ++y) {
    for (size_t x = 0; x != width; ++x) {
      result_image->SetValue(x, y, l_data->Value(x, y) / r_data->Value(x, y));
    }
  }
  lhs.SetImage(0, std::move(result_image));
}

}  // namespace

Image2DCPtr TimeFrequencyData::GetAbsoluteFromComplex(
    const Image2DCPtr& real, const Image2DCPtr& imag) const {
  return Image2DPtr(FFTTools::CreateAbsoluteImage(*real, *imag));
}

Image2DCPtr TimeFrequencyData::GetSum(const Image2DCPtr& left,
                                      const Image2DCPtr& right) const {
  return StokesImager::CreateSum(left, right);
}

Image2DCPtr TimeFrequencyData::GetNegatedSum(const Image2DCPtr& left,
                                             const Image2DCPtr& right) const {
  return StokesImager::CreateNegatedSum(left, right);
}

Image2DCPtr TimeFrequencyData::GetDifference(const Image2DCPtr& left,
                                             const Image2DCPtr& right) const {
  return StokesImager::CreateDifference(left, right);
}

Image2DCPtr TimeFrequencyData::getSinglePhaseFromTwoPolPhase(
    size_t polA, size_t polB) const {
  return StokesImager::CreateAvgPhase(_data[polA]._images[0],
                                      _data[polB]._images[0]);
}

Mask2DCPtr TimeFrequencyData::GetCombinedMask() const {
  if (MaskCount() == 0) {
    return GetSetMask<false>();
  } else if (MaskCount() == 1) {
    return GetMask(0);
  } else {
    Mask2DPtr mask(new Mask2D(*GetMask(0)));
    size_t i = 0;
    while (i != MaskCount()) {
      const Mask2DCPtr& curMask = GetMask(i);
      for (unsigned y = 0; y < mask->Height(); ++y) {
        for (unsigned x = 0; x < mask->Width(); ++x) {
          const bool v = curMask->Value(x, y);
          if (v) mask->SetValue(x, y, true);
        }
      }
      ++i;
    }
    return mask;
  }
}

TimeFrequencyData TimeFrequencyData::MakeZeroLinearData(size_t width,
                                                        size_t height) {
  const Image2DPtr zero = Image2D::CreateSetImagePtr(width, height, 0.0);
  TimeFrequencyData data;
  data._data.resize(4);
  data._data[0] =
      PolarizedTimeFrequencyData(aocommon::Polarization::XX, zero, zero);
  data._data[1] =
      PolarizedTimeFrequencyData(aocommon::Polarization::XY, zero, zero);
  data._data[2] =
      PolarizedTimeFrequencyData(aocommon::Polarization::YX, zero, zero);
  data._data[3] =
      PolarizedTimeFrequencyData(aocommon::Polarization::YY, zero, zero);
  return data;
}

TimeFrequencyData TimeFrequencyData::Make(
    enum ComplexRepresentation representation) const {
  if (representation == _complexRepresentation) {
    return TimeFrequencyData(*this);
  } else if (_complexRepresentation == ComplexParts) {
    TimeFrequencyData data;
    data._complexRepresentation = representation;
    data._data.resize(_data.size());
    for (size_t i = 0; i != _data.size(); ++i) {
      const PolarizedTimeFrequencyData& source = _data[i];
      PolarizedTimeFrequencyData& dest = data._data[i];
      dest._polarization = source._polarization;
      dest._flagging = source._flagging;
      switch (representation) {
        case RealPart:
          dest._images[0] = source._images[0];
          break;
        case ImaginaryPart:
          dest._images[0] = source._images[1];
          break;
        case AmplitudePart:
          dest._images[0] =
              GetAbsoluteFromComplex(source._images[0], source._images[1]);
          break;
        case PhasePart:
          dest._images[0] = StokesImager::CreateAvgPhase(source._images[0],
                                                         source._images[1]);
          break;
        case ComplexParts:
          break;  // already handled above.
      }
    }
    return data;
  } else if (representation == ComplexParts &&
             _complexRepresentation == AmplitudePart) {
    return MakeFromComplexCombination(*this, *this);
  } else {
    throw std::runtime_error(
        "Request for time/frequency data with a phase representation that can "
        "not be extracted from the source (source is not complex)");
  }
}

TimeFrequencyData TimeFrequencyData::MakeFromComplexCombination(
    const TimeFrequencyData& real, const TimeFrequencyData& imaginary) {
  if (real.ComplexRepresentation() == ComplexParts ||
      imaginary.ComplexRepresentation() == ComplexParts)
    throw std::runtime_error(
        "Trying to create complex TF data from incorrect phase "
        "representations");
  if (real.Polarizations() != imaginary.Polarizations())
    throw std::runtime_error(
        "Combining real/imaginary time frequency data from different "
        "polarisations");
  TimeFrequencyData data;
  data._data.resize(real._data.size());
  data._complexRepresentation = ComplexParts;
  for (size_t i = 0; i != real._data.size(); ++i) {
    data._data[i]._polarization = real._data[i]._polarization;
    data._data[i]._images[0] = real._data[i]._images[0];
    data._data[i]._images[1] = imaginary._data[i]._images[0];
    data._data[i]._flagging = real._data[i]._flagging;
  }
  return data;
}

TimeFrequencyData TimeFrequencyData::MakeFromPolarizationCombination(
    const TimeFrequencyData& xx, const TimeFrequencyData& xy,
    const TimeFrequencyData& yx, const TimeFrequencyData& yy) {
  if (xx.ComplexRepresentation() != xy.ComplexRepresentation() ||
      xx.ComplexRepresentation() != yx.ComplexRepresentation() ||
      xx.ComplexRepresentation() != yy.ComplexRepresentation())
    throw std::runtime_error(
        "Trying to create dipole time frequency combination from data with "
        "different phase representations!");

  TimeFrequencyData data;
  data._data.resize(4);
  data._complexRepresentation = xx._complexRepresentation;
  for (size_t i = 0; i != xx._data.size(); ++i) {
    data._data[0] = xx._data[0];
    data._data[1] = xy._data[0];
    data._data[2] = yx._data[0];
    data._data[3] = yy._data[0];
  }
  return data;
}

TimeFrequencyData TimeFrequencyData::MakeFromPolarizationCombination(
    const TimeFrequencyData& first, const TimeFrequencyData& second) {
  if (first.IsEmpty()) return second;
  if (second.IsEmpty()) return first;
  if (first.ComplexRepresentation() != second.ComplexRepresentation())
    throw std::runtime_error(
        "Trying to create polarization combination from data with different "
        "phase representations!");

  TimeFrequencyData data;
  data._data = first._data;
  data._complexRepresentation = first._complexRepresentation;
  data._data.insert(data._data.end(), second._data.begin(), second._data.end());
  return data;
}

void TimeFrequencyData::SetImagesToZero() {
  if (!IsEmpty()) {
    const Image2DPtr zeroImage =
        Image2D::CreateZeroImagePtr(ImageWidth(), ImageHeight());
    const Mask2DPtr mask =
        Mask2D::CreateSetMaskPtr<false>(ImageWidth(), ImageHeight());
    for (PolarizedTimeFrequencyData& data : _data) {
      data._images[0] = zeroImage;
      if (data._images[1]) data._images[1] = zeroImage;
      data._flagging = mask;
    }
  }
}

void TimeFrequencyData::MultiplyImages(long double factor) {
  for (PolarizedTimeFrequencyData& data : _data) {
    if (data._images[0]) {
      const Image2DPtr newImage(new Image2D(*data._images[0]));
      newImage->MultiplyValues(factor);
      data._images[0] = newImage;
    }
    if (data._images[1]) {
      const Image2DPtr newImage(new Image2D(*data._images[1]));
      newImage->MultiplyValues(factor);
      data._images[1] = newImage;
    }
  }
}

void TimeFrequencyData::JoinMask(const TimeFrequencyData& other) {
  if (other.MaskCount() == 0) {
    // Nothing to be done; other has no flags
  } else if (other.MaskCount() == MaskCount()) {
    for (size_t i = 0; i < MaskCount(); ++i) {
      Mask2D mask(*GetMask(i));
      mask.Join(*other.GetMask(i));
      SetMask(i, Mask2DPtr(new Mask2D(mask)));
    }
  } else if (other.MaskCount() == 1) {
    if (MaskCount() == 0) {
      for (size_t i = 0; i != _data.size(); ++i)
        _data[i]._flagging = other._data[0]._flagging;
    } else {
      for (size_t i = 0; i < MaskCount(); ++i) {
        Mask2D mask(*GetMask(i));
        mask.Join(*other.GetMask(0));
        SetMask(i, Mask2DPtr(new Mask2D(mask)));
      }
    }
  } else if (MaskCount() == 1) {
    Mask2D mask(*GetMask(0));
    mask.Join(*other.GetSingleMask());
    SetMask(0, Mask2DPtr(new Mask2D(mask)));
  } else if (MaskCount() == 0 && _data.size() == other._data.size()) {
    for (size_t i = 0; i != _data.size(); ++i)
      _data[i]._flagging = other._data[i]._flagging;
  } else {
    throw std::runtime_error(
        "Joining time frequency flagging with incompatible structures");
  }
}

std::vector<std::complex<num_t>> ToComplexVector(
    const TimeFrequencyData& tf_data) {
  if (tf_data.ComplexRepresentation() != TimeFrequencyData::ComplexParts)
    throw std::runtime_error(
        "Can't convert non-complex data into a complex vector");
  const size_t n_pol = tf_data.PolarizationCount();
  const size_t width = tf_data.ImageWidth();
  const size_t height = tf_data.ImageHeight();
  std::vector<std::complex<num_t>> data(n_pol * width * height);
  for (size_t pol = 0; pol != n_pol; ++pol) {
    const TimeFrequencyData pol_data = tf_data.MakeFromPolarizationIndex(pol);
    const Image2DCPtr real = pol_data.GetImage(0);
    const Image2DCPtr imaginary = pol_data.GetImage(1);
    size_t index = pol * width * height;
    for (size_t y = 0; y != height; ++y) {
      for (size_t x = 0; x != width; ++x) {
        data[index] = {real->Value(x, y), imaginary->Value(x, y)};
        ++index;
      }
    }
  }
  return data;
}

TimeFrequencyData ElementWiseDivide(const TimeFrequencyData& lhs,
                                    const TimeFrequencyData& rhs) {
  if (lhs.ImageCount() != rhs.ImageCount() ||
      lhs.ComplexRepresentation() != rhs.ComplexRepresentation()) {
    throw std::runtime_error(
        "Can not element-wise divide time-frequency data: inputs do not have "
        "the same number of polarizations or complex representation!");
  }
  if (lhs.ImageWidth() != rhs.ImageWidth() ||
      lhs.ImageHeight() != rhs.ImageHeight())
    throw std::runtime_error(
        "Can not element-wise divide time-frequency data: inputs have "
        "different sizes.");
  TimeFrequencyData data(lhs);
  for (size_t pol_index = 0; pol_index != data.PolarizationCount();
       ++pol_index) {
    TimeFrequencyData lhs_pol = lhs.MakeFromPolarizationIndex(pol_index);
    const TimeFrequencyData rhs_pol = rhs.MakeFromPolarizationIndex(pol_index);
    if (lhs_pol.ImageCount() == 2) {
      DivideComplexSinglePolarization(lhs_pol, rhs_pol);
    } else {
      DivideRealSinglePolarization(lhs_pol, rhs_pol);
    }
    data.SetPolarizationData(pol_index, std::move(lhs_pol));
  }
  return data;
}

TimeFrequencyData ElementWiseNorm(const TimeFrequencyData& data) {
  if (data.ComplexRepresentation() == TimeFrequencyData::ComplexParts) {
    std::vector<Image2DCPtr> norm_images;
    norm_images.reserve(data.PolarizationCount());
    for (size_t pol_index = 0; pol_index != data.PolarizationCount();
         ++pol_index) {
      TimeFrequencyData pol = data.MakeFromPolarizationIndex(pol_index);
      Image2DCPtr real = pol.GetImage(0);
      Image2DCPtr imaginary = pol.GetImage(1);
      const size_t width = pol.ImageWidth();
      const size_t height = pol.ImageHeight();
      Image2DPtr norm_image = Image2D::CreateUnsetImagePtr(width, height);
      for (size_t y = 0; y != height; ++y) {
        for (size_t x = 0; x != width; ++x) {
          const std::complex<num_t> z(real->Value(x, y),
                                      imaginary->Value(x, y));
          norm_image->SetValue(x, y, std::norm(z));
        }
      }
      norm_images.emplace_back(std::move(norm_image));
    }
    return TimeFrequencyData(TimeFrequencyData::AmplitudePart,
                             data.Polarizations().data(),
                             data.PolarizationCount(), norm_images.data());
  } else {
    TimeFrequencyData result = data;
    for (size_t pol_index = 0; pol_index != data.PolarizationCount();
         ++pol_index) {
      TimeFrequencyData pol = data.MakeFromPolarizationIndex(pol_index);
      Image2DCPtr values = pol.GetImage(0);
      const size_t width = values->Width();
      const size_t height = values->Height();
      Image2DPtr new_values = Image2D::CreateUnsetImagePtr(width, height);
      for (size_t y = 0; y != height; ++y) {
        for (size_t x = 0; x != width; ++x) {
          new_values->SetValue(x, y, std::norm(values->Value(x, y)));
        }
      }
      pol.SetImage(0, new_values);
      result.SetPolarizationData(pol_index, pol);
    }
    return result;
  }
}

TimeFrequencyData ElementWiseSqrt(const TimeFrequencyData& data) {
  TimeFrequencyData result = data;
  for (size_t pol_index = 0; pol_index != data.PolarizationCount();
       ++pol_index) {
    TimeFrequencyData pol = data.MakeFromPolarizationIndex(pol_index);
    const size_t width = pol.ImageWidth();
    const size_t height = pol.ImageHeight();
    if (pol.ImageCount() == 2) {
      Image2DCPtr real = pol.GetImage(0);
      Image2DCPtr imaginary = pol.GetImage(1);
      Image2DPtr new_real = Image2D::CreateUnsetImagePtr(width, height);
      Image2DPtr new_imaginary = Image2D::CreateUnsetImagePtr(width, height);
      for (size_t y = 0; y != height; ++y) {
        for (size_t x = 0; x != width; ++x) {
          const std::complex<num_t> z(real->Value(x, y),
                                      imaginary->Value(x, y));
          const std::complex<num_t> sqrt = std::sqrt(z);
          new_real->SetValue(x, y, sqrt.real());
          new_imaginary->SetValue(x, y, sqrt.imag());
        }
      }
      pol.SetImage(0, new_real);
      pol.SetImage(1, new_imaginary);
    } else {
      Image2DCPtr values = pol.GetImage(0);
      Image2DPtr new_values = Image2D::CreateUnsetImagePtr(width, height);
      for (size_t y = 0; y != height; ++y) {
        for (size_t x = 0; x != width; ++x) {
          new_values->SetValue(x, y, std::sqrt(values->Value(x, y)));
        }
      }
      pol.SetImage(0, new_values);
    }
    result.SetPolarizationData(pol_index, pol);
  }
  return result;
}
