#include <algorithm>
#include <deque>
#include <limits>
#include <memory>

#include <boost/numeric/conversion/bounds.hpp>

#include "../util/rng.h"

#include "thresholdtools.h"

namespace algorithms {

void ThresholdTools::MeanAndStdDev(const Image2D* image, const Mask2D* mask,
                                   num_t& mean, num_t& stddev) {
  // Calculate mean
  mean = 0.0;
  size_t count = 0;
  for (size_t y = 0; y < image->Height(); ++y) {
    for (size_t x = 0; x < image->Width(); ++x) {
      if (!mask->Value(x, y) && std::isfinite(image->Value(x, y))) {
        const num_t value = image->Value(x, y);
        mean += value;
        count++;
      }
    }
  }
  mean /= (num_t)count;
  // Calculate variance
  stddev = 0.0;
  count = 0;
  for (size_t y = 0; y < image->Height(); ++y) {
    for (size_t x = 0; x < image->Width(); ++x) {
      if (!mask->Value(x, y) && std::isfinite(image->Value(x, y))) {
        const num_t value = image->Value(x, y);
        stddev += (value - mean) * (value - mean);
        count++;
      }
    }
  }
  stddev = sqrtn(stddev / (num_t)count);
}

void ThresholdTools::WinsorizedMeanAndStdDev(const Image2D* image, num_t& mean,
                                             num_t& stddev) {
  const size_t size = image->Width() * image->Height();
  num_t* data = new num_t[size];
  image->CopyData(data);
  std::sort(data, data + size, numLessThanOperator);
  const size_t lowIndex = (size_t)floor(0.1 * size);
  const size_t highIndex = (size_t)ceil(0.9 * size) - 1;
  const num_t lowValue = data[lowIndex];
  const num_t highValue = data[highIndex];
  delete[] data;

  // Calculate mean
  mean = 0.0;
  size_t count = 0;
  for (size_t y = 0; y < image->Height(); ++y) {
    for (size_t x = 0; x < image->Width(); ++x) {
      if (std::isfinite(image->Value(x, y))) {
        const num_t value = image->Value(x, y);
        if (value < lowValue)
          mean += lowValue;
        else if (value > highValue)
          mean += highValue;
        else
          mean += value;
        count++;
      }
    }
  }
  if (count > 0) mean /= (num_t)count;
  // Calculate variance
  stddev = 0.0;
  count = 0;
  for (size_t y = 0; y < image->Height(); ++y) {
    for (size_t x = 0; x < image->Width(); ++x) {
      if (std::isfinite(image->Value(x, y))) {
        const num_t value = image->Value(x, y);
        if (value < lowValue)
          stddev += (lowValue - mean) * (lowValue - mean);
        else if (value > highValue)
          stddev += (highValue - mean) * (highValue - mean);
        else
          stddev += (value - mean) * (value - mean);
        count++;
      }
    }
  }
  if (count > 0)
    stddev = sqrtn(1.54 * stddev / (num_t)count);
  else
    stddev = 0.0;
}

template <typename T>
void ThresholdTools::TrimmedMeanAndStdDev(const std::vector<T>& input, T& mean,
                                          T& stddev) {
  if (input.size() == 1) {
    mean = input[0];
    stddev = 0.0;
    return;
  } else if (input.size() == 0) {
    mean = 0;
    stddev = 0.0;
    return;
  }
  std::vector<T> data(input);
  std::sort(data.begin(), data.end(), numLessThanOperator);
  const size_t lowIndex = (size_t)floor(0.25 * data.size());
  const size_t highIndex = (size_t)ceil(0.75 * data.size()) - 1;
  T lowValue = data[lowIndex];
  T highValue = data[highIndex];

  // Calculate mean
  mean = 0.0;
  size_t count = 0;
  for (typename std::vector<T>::const_iterator i = data.begin();
       i != data.end(); ++i) {
    if (std::isfinite(*i) && *i > lowValue && *i < highValue) {
      mean += *i;
      ++count;
    }
  }
  if (count > 0) mean /= (T)count;
  // Calculate variance
  stddev = 0.0;
  count = 0;
  for (typename std::vector<T>::const_iterator i = data.begin();
       i != data.end(); ++i) {
    if (std::isfinite(*i) && *i >= lowValue && *i <= highValue) {
      stddev += (*i - mean) * (*i - mean);
      ++count;
    }
  }
  if (count > 0)
    stddev = sqrt(3.3 * stddev / (T)count);
  else
    stddev = 0.0;
}

template void ThresholdTools::TrimmedMeanAndStdDev(
    const std::vector<num_t>& input, num_t& mean, num_t& stddev);
template void ThresholdTools::TrimmedMeanAndStdDev(
    const std::vector<double>& input, double& mean, double& stddev);

template <typename T>
void ThresholdTools::WinsorizedMeanAndStdDev(const std::vector<T>& input,
                                             T& mean, T& stddev) {
  if (input.empty()) {
    mean = 0.0;
    stddev = 0.0;
  } else {
    std::vector<T> data(input);
    std::sort(data.begin(), data.end(), numLessThanOperator);
    const size_t lowIndex = (size_t)floor(0.1 * data.size());
    const size_t highIndex = (size_t)ceil(0.9 * data.size()) - 1;
    T lowValue = data[lowIndex];
    T highValue = data[highIndex];

    // Calculate mean
    mean = 0.0;
    size_t count = 0;
    for (typename std::vector<T>::const_iterator i = data.begin();
         i != data.end(); ++i) {
      if (std::isfinite(*i)) {
        if (*i < lowValue)
          mean += lowValue;
        else if (*i > highValue)
          mean += highValue;
        else
          mean += *i;
        count++;
      }
    }
    if (count > 0) mean /= (T)count;
    // Calculate variance
    stddev = 0.0;
    count = 0;
    for (typename std::vector<T>::const_iterator i = data.begin();
         i != data.end(); ++i) {
      if (std::isfinite(*i)) {
        if (*i < lowValue)
          stddev += (lowValue - mean) * (lowValue - mean);
        else if (*i > highValue)
          stddev += (highValue - mean) * (highValue - mean);
        else
          stddev += (*i - mean) * (*i - mean);
        count++;
      }
    }
    if (count > 0)
      stddev = sqrt(1.54 * stddev / (T)count);
    else
      stddev = 0.0;
  }
}

template void ThresholdTools::WinsorizedMeanAndStdDev(
    const std::vector<num_t>& input, num_t& mean, num_t& stddev);
template void ThresholdTools::WinsorizedMeanAndStdDev(
    const std::vector<double>& input, double& mean, double& stddev);

void ThresholdTools::WinsorizedMeanAndStdDev(const Image2D* image,
                                             const Mask2D* mask, num_t& mean,
                                             num_t& stddev) {
  num_t* data = new num_t[image->Width() * image->Height()];
  size_t unflaggedCount = 0;
  for (size_t y = 0; y < image->Height(); ++y) {
    for (size_t x = 0; x < image->Width(); ++x) {
      const num_t val = image->Value(x, y);
      if (!mask->Value(x, y) && std::isfinite(val)) {
        data[unflaggedCount] = image->Value(x, y);
        ++unflaggedCount;
      }
    }
  }
  const size_t lowIndex = (size_t)floor(0.1 * unflaggedCount);
  size_t highIndex = (size_t)ceil(0.9 * unflaggedCount);
  if (highIndex > 0) --highIndex;
  std::nth_element(data, data + lowIndex, data + unflaggedCount,
                   numLessThanOperator);
  const num_t lowValue = data[lowIndex];
  std::nth_element(data, data + highIndex, data + unflaggedCount,
                   numLessThanOperator);
  const num_t highValue = data[highIndex];

  // Calculate mean
  mean = 0.0;
  for (size_t i = 0; i < unflaggedCount; ++i) {
    const num_t value = data[i];
    if (value < lowValue)
      mean += lowValue;
    else if (value > highValue)
      mean += highValue;
    else
      mean += value;
  }
  if (unflaggedCount > 0) mean /= (num_t)unflaggedCount;
  // Calculate variance
  stddev = 0.0;
  for (size_t i = 0; i < unflaggedCount; ++i) {
    const num_t value = data[i];
    if (value < lowValue)
      stddev += (lowValue - mean) * (lowValue - mean);
    else if (value > highValue)
      stddev += (highValue - mean) * (highValue - mean);
    else
      stddev += (value - mean) * (value - mean);
  }
  delete[] data;
  if (unflaggedCount > 0)
    stddev = sqrtn(1.54 * stddev / (num_t)unflaggedCount);
  else
    stddev = 0.0;
}

void ThresholdTools::WinsorizedMeanAndStdDev(const Image2D* image,
                                             const Mask2D* maskA,
                                             const Mask2D* maskB, num_t& mean,
                                             num_t& stddev) {
  const std::unique_ptr<num_t[]> data(
      new num_t[image->Width() * image->Height()]);
  size_t unflaggedCount = 0;
  for (size_t y = 0; y < image->Height(); ++y) {
    for (size_t x = 0; x < image->Width(); ++x) {
      const num_t val = image->Value(x, y);
      if (!maskA->Value(x, y) && !maskB->Value(x, y) && std::isfinite(val)) {
        data[unflaggedCount] = image->Value(x, y);
        ++unflaggedCount;
      }
    }
  }
  const size_t lowIndex = (size_t)floor(0.1 * unflaggedCount);
  size_t highIndex = (size_t)ceil(0.9 * unflaggedCount);
  if (highIndex > 0) --highIndex;
  std::nth_element(data.get(), data.get() + lowIndex,
                   data.get() + unflaggedCount, numLessThanOperator);
  const num_t lowValue = data[lowIndex];
  std::nth_element(data.get(), data.get() + highIndex,
                   data.get() + unflaggedCount, numLessThanOperator);
  const num_t highValue = data[highIndex];

  // Calculate mean
  mean = 0.0;
  for (size_t i = 0; i < unflaggedCount; ++i) {
    const num_t value = data[i];
    if (value < lowValue)
      mean += lowValue;
    else if (value > highValue)
      mean += highValue;
    else
      mean += value;
  }
  if (unflaggedCount > 0) mean /= (num_t)unflaggedCount;
  // Calculate variance
  stddev = 0.0;
  for (size_t i = 0; i < unflaggedCount; ++i) {
    const num_t value = data[i];
    if (value < lowValue)
      stddev += (lowValue - mean) * (lowValue - mean);
    else if (value > highValue)
      stddev += (highValue - mean) * (highValue - mean);
    else
      stddev += (value - mean) * (value - mean);
  }
  if (unflaggedCount > 0)
    stddev = sqrtn(1.54 * stddev / (num_t)unflaggedCount);
  else
    stddev = 0.0;
}

template <typename T>
double ThresholdTools::WinsorizedRMS(
    const std::vector<std::complex<T>>& input) {
  if (input.empty()) {
    return 0.0;
  } else {
    std::vector<std::complex<T>> data(input);
    std::sort(data.begin(), data.end(), complexLessThanOperator<T>);
    const size_t lowIndex = (size_t)floor(0.1 * data.size());
    const size_t highIndex = (size_t)ceil(0.9 * data.size()) - 1;
    const std::complex<T> lowValue = data[lowIndex];
    const std::complex<T> highValue = data[highIndex];

    // Calculate RMS
    double rms = 0.0;
    size_t count = 0;
    for (const std::complex<T>& val : data) {
      if (std::isfinite(val.real()) && std::isfinite(val.imag())) {
        if (complexLessThanOperator<T>(val, lowValue))
          rms += (lowValue * std::conj(lowValue)).real();
        else if (complexLessThanOperator<T>(highValue, val))
          rms += (highValue * std::conj(highValue)).real();
        else
          rms += (val * std::conj(val)).real();
        count++;
      }
    }
    if (count > 0)
      return sqrt(1.54 * rms / (T)count);
    else
      return 0.0;
  }
}

template double ThresholdTools::WinsorizedRMS(
    const std::vector<std::complex<float>>& input);
template double ThresholdTools::WinsorizedRMS(
    const std::vector<std::complex<double>>& input);

num_t ThresholdTools::MinValue(const Image2D* image, const Mask2D* mask) {
  num_t minValue = std::numeric_limits<num_t>::max();
  for (size_t y = 0; y < image->Height(); ++y) {
    for (size_t x = 0; x < image->Width(); ++x) {
      if (!mask->Value(x, y) && std::isfinite(image->Value(x, y)) &&
          image->Value(x, y) < minValue)
        minValue = image->Value(x, y);
    }
  }
  return minValue;
}

num_t ThresholdTools::MaxValue(const Image2D* image, const Mask2D* mask) {
  num_t maxValue = boost::numeric::bounds<num_t>::lowest();
  for (size_t y = 0; y < image->Height(); ++y) {
    for (size_t x = 0; x < image->Width(); ++x) {
      if (!mask->Value(x, y) && std::isfinite(image->Value(x, y)) &&
          image->Value(x, y) > maxValue)
        maxValue = image->Value(x, y);
    }
  }
  return maxValue;
}

void ThresholdTools::SetFlaggedValuesToZero(Image2D* dest, const Mask2D* mask) {
  for (size_t y = 0; y < dest->Height(); ++y) {
    for (size_t x = 0; x < dest->Width(); ++x) {
      if (mask->Value(x, y)) dest->SetValue(x, y, 0.0);
    }
  }
}

void ThresholdTools::CountMaskLengths(const Mask2D* mask, int* lengths,
                                      size_t lengthsSize) {
  for (size_t i = 0; i < lengthsSize; ++i) lengths[i] = 0;
  int *horizontal, *vertical;
  horizontal = new int[mask->Width() * mask->Height()];
  vertical = new int[mask->Width() * mask->Height()];
  size_t y = 0, index = 0;
  // Count horizontally lengths
  while (y < mask->Height()) {
    size_t x = 0;
    while (x < mask->Width()) {
      if (mask->Value(x, y)) {
        const size_t xStart = x;
        do {
          ++x;
          ++index;
        } while (x < mask->Width() && mask->Value(x, y));
        for (size_t i = 0; i < x - xStart; ++i)
          horizontal[index - (x - xStart) + i] = x - xStart;
      } else {
        horizontal[index] = 0;
        ++x;
        ++index;
      }
    }
    ++y;
  }
  // Count vertically lengths
  size_t x = 0;
  while (x < mask->Width()) {
    size_t y = 0;
    while (y < mask->Height()) {
      if (mask->Value(x, y)) {
        const size_t yStart = y;
        while (y < mask->Height() && mask->Value(x, y)) {
          ++y;
        }
        for (size_t i = yStart; i < y; ++i)
          vertical[i * mask->Width() + x] = y - yStart;
      } else {
        vertical[y * mask->Width() + x] = 0;
        ++y;
      }
    }
    ++x;
  }
  // Count the horizontal distribution
  index = 0;
  for (size_t y = 0; y < mask->Height(); ++y) {
    size_t x = 0;
    while (x < mask->Width()) {
      if (horizontal[index] != 0) {
        const int count = horizontal[index];
        bool dominant = false;
        for (int i = 0; i < count; ++i) {
          if (count >= vertical[index + i]) {
            dominant = true;
            break;
          }
        }
        if (dominant && (size_t)count - 1 < lengthsSize) ++lengths[count - 1];
        x += count;
        index += count;
      } else {
        ++index;
        ++x;
      }
    }
  }
  // Count the vertical distribution
  for (size_t x = 0; x < mask->Width(); ++x) {
    size_t y = 0;
    while (y < mask->Height()) {
      if (vertical[y * mask->Width() + x] != 0) {
        const int count = vertical[y * mask->Width() + x];
        bool dominant = false;
        for (int i = 0; i < count; ++i) {
          if (count >= horizontal[(y + i) * mask->Width() + x]) {
            dominant = true;
            break;
          }
        }
        if (dominant && (size_t)count - 1 < lengthsSize) ++lengths[count - 1];
        y += count;
      } else {
        ++y;
      }
    }
  }
  delete[] vertical;
  delete[] horizontal;
}

num_t ThresholdTools::Mode(const Image2D* image, const Mask2D* mask) {
  num_t mode = 0.0;
  size_t count = 0;
  for (size_t y = 0; y < image->Height(); ++y) {
    for (size_t x = 0; x < image->Width(); ++x) {
      const num_t value = image->Value(x, y);
      if (!mask->Value(x, y) && std::isfinite(value)) {
        mode += value * value;
        count++;
      }
    }
  }
  return sqrtn(mode / (2.0 * (num_t)count));
}

numl_t ThresholdTools::Sum(const Image2D* image, const Mask2D* mask) {
  numl_t sum = 0.0;
  for (size_t y = 0; y < image->Height(); ++y) {
    for (size_t x = 0; x < image->Width(); ++x) {
      if (!mask->Value(x, y)) sum += image->Value(x, y);
    }
  }
  return sum;
}

numl_t ThresholdTools::RMS(const Image2D* image, const Mask2D* mask) {
  numl_t mode = 0.0;
  size_t count = 0;
  for (size_t y = 0; y < image->Height(); ++y) {
    for (size_t x = 0; x < image->Width(); ++x) {
      const num_t value = image->Value(x, y);
      if (!mask->Value(x, y) && std::isfinite(value)) {
        mode += (numl_t)value * (numl_t)value;
        count++;
      }
    }
  }
  return sqrtnl(mode / (numl_t)count);
}

num_t ThresholdTools::WinsorizedMode(const Image2D* image, const Mask2D* mask) {
  num_t* data = new num_t[image->Width() * image->Height()];
  size_t unflaggedCount = 0;
  for (size_t y = 0; y < image->Height(); ++y) {
    for (size_t x = 0; x < image->Width(); ++x) {
      const num_t val = image->Value(x, y);
      if (!mask->Value(x, y) && std::isfinite(val)) {
        data[unflaggedCount] = val;
        ++unflaggedCount;
      }
    }
  }
  const size_t highIndex = (size_t)floor(0.9 * unflaggedCount);
  std::nth_element(data, data + highIndex, data + unflaggedCount);
  const num_t highValue = data[highIndex];

  num_t mode = 0.0;
  for (size_t i = 0; i < unflaggedCount; ++i) {
    const num_t value = data[i];
    if (value > highValue)
      mode += highValue * highValue;
    else
      mode += value * value;
  }
  delete[] data;
  // The correction factor 1.0541 was found by running simulations
  // It corresponds with the correction factor needed when winsorizing 10% of
  // the data, meaning that the highest 10% is set to the value exactly at the
  // 90%/10% limit.
  if (unflaggedCount > 0)
    return sqrtn(mode / (2.0 * (num_t)unflaggedCount)) * 1.0541;
  else
    return 0.0;
}

num_t ThresholdTools::WinsorizedMode(const Image2D* image, const Mask2D* maskA,
                                     const Mask2D* maskB) {
  const std::unique_ptr<num_t[]> data(
      new num_t[image->Width() * image->Height()]);
  size_t unflaggedCount = 0;
  for (size_t y = 0; y < image->Height(); ++y) {
    for (size_t x = 0; x < image->Width(); ++x) {
      const num_t val = image->Value(x, y);
      if (!maskA->Value(x, y) && !maskB->Value(x, y) && std::isfinite(val)) {
        data[unflaggedCount] = val;
        ++unflaggedCount;
      }
    }
  }
  const size_t highIndex = (size_t)floor(0.9 * unflaggedCount);
  std::nth_element(data.get(), data.get() + highIndex,
                   data.get() + unflaggedCount);
  const num_t highValue = data[highIndex];

  num_t mode = 0.0;
  for (size_t i = 0; i < unflaggedCount; ++i) {
    const num_t value = data[i];
    if (value > highValue)
      mode += highValue * highValue;
    else
      mode += value * value;
  }
  if (unflaggedCount > 0)
    return sqrtn(mode / (2.0 * (num_t)unflaggedCount)) * 1.0541;
  else
    return 0.0;
}

num_t ThresholdTools::WinsorizedMode(const Image2D* image) {
  const size_t size = image->Width() * image->Height();
  num_t* data = new num_t[size];
  image->CopyData(data);
  std::sort(data, data + size, numLessThanOperator);
  const size_t highIndex = (size_t)ceil(0.9 * size) - 1;
  const num_t highValue = data[highIndex];
  delete[] data;

  num_t mode = 0.0;
  for (size_t y = 0; y < image->Height(); ++y) {
    for (size_t x = 0; x < image->Width(); ++x) {
      const num_t value = image->Value(x, y);
      if (value > highValue || -value > highValue)
        mode += highValue * highValue;
      else
        mode += value * value;
    }
  }
  // The correction factor 1.0541 was found by running simulations
  // It corresponds with the correction factor needed when winsorizing 10% of
  // the data, meaning that the highest 10% is set to the value exactly at the
  // 90%/10% limit.
  if (size > 0)
    return sqrtn(mode / (2.0L * (num_t)size)) * 1.0541L;
  else
    return 0.0;
}

void ThresholdTools::FilterConnectedSamples(Mask2D* mask,
                                            size_t minConnectedSampleArea,
                                            bool eightConnected) {
  for (size_t y = 0; y < mask->Height(); ++y) {
    for (size_t x = 0; x < mask->Width(); ++x)
      if (mask->Value(x, y))
        FilterConnectedSample(mask, x, y, minConnectedSampleArea,
                              eightConnected);
  }
}

struct ConnectedAreaCoord {
  ConnectedAreaCoord(size_t _x, size_t _y) throw() {
    x = _x;
    y = _y;
  }
  size_t x, y;
};

void ThresholdTools::FilterConnectedSample(Mask2D* mask, size_t x, size_t y,
                                           size_t minConnectedSampleArea,
                                           bool eightConnected) {
  std::deque<ConnectedAreaCoord> tosearch, changed;
  tosearch.push_back(ConnectedAreaCoord(x, y));
  size_t count = 0;

  do {
    const ConnectedAreaCoord c = tosearch.front();
    tosearch.pop_front();
    if (mask->Value(c.x, c.y)) {
      mask->SetValue(c.x, c.y, false);
      changed.push_back(ConnectedAreaCoord(c.x, c.y));
      if (c.x > 0) tosearch.push_back(ConnectedAreaCoord(c.x - 1, c.y));
      if (c.x < mask->Width() - 1)
        tosearch.push_back(ConnectedAreaCoord(c.x + 1, c.y));
      if (c.y > 0) tosearch.push_back(ConnectedAreaCoord(c.x, c.y - 1));
      if (c.y < mask->Height() - 1)
        tosearch.push_back(ConnectedAreaCoord(c.x, c.y + 1));
      if (eightConnected) {
        if (c.x > 0 && c.y > 0)
          tosearch.push_back(ConnectedAreaCoord(c.x - 1, c.y - 1));
        if (c.x < mask->Width() - 1 && c.y < mask->Height() - 1)
          tosearch.push_back(ConnectedAreaCoord(c.x + 1, c.y + 1));
        if (c.x < mask->Width() - 1 && c.y > 0)
          tosearch.push_back(ConnectedAreaCoord(c.x + 1, c.y - 1));
        if (c.x > 0 && c.y < mask->Height() - 1)
          tosearch.push_back(ConnectedAreaCoord(c.x - 1, c.y + 1));
      }
      ++count;
    }
  } while (tosearch.size() != 0 && count < minConnectedSampleArea);

  if (count >= minConnectedSampleArea) {
    while (changed.size() != 0) {
      const ConnectedAreaCoord c = changed.front();
      changed.pop_front();
      mask->SetValue(c.x, c.y, true);
    }
  }
}

void ThresholdTools::UnrollPhase(Image2D* image) {
  for (size_t y = 0; y < image->Height(); ++y) {
    num_t prev = image->Value(0, y);
    for (size_t x = 1; x < image->Width(); ++x) {
      num_t val = image->Value(x, y);
      while (val - prev > M_PIn) val -= 2.0L * M_PIn;
      while (prev - val > M_PIn) val += 2.0L * M_PIn;
      image->SetValue(x, y, val);
      prev = val;
    }
  }
}

Image2DPtr ThresholdTools::ShrinkHorizontally(size_t factor,
                                              const Image2D* input,
                                              const Mask2D* mask) {
  const size_t oldWidth = input->Width();
  const size_t newWidth = (oldWidth + factor - 1) / factor;

  Image2DPtr newImage = Image2D::CreateUnsetImagePtr(newWidth, input->Height());

  for (size_t x = 0; x < newWidth; ++x) {
    size_t avgSize = factor;
    if (avgSize + x * factor > oldWidth) avgSize = oldWidth - x * factor;

    for (size_t y = 0; y < input->Height(); ++y) {
      size_t count = 0;
      num_t sum = 0.0;
      for (size_t binX = 0; binX < avgSize; ++binX) {
        const size_t curX = x * factor + binX;
        if (!mask->Value(curX, y)) {
          sum += input->Value(curX, y);
          ++count;
        }
      }
      if (count == 0) {
        sum = 0.0;
        for (size_t binX = 0; binX < avgSize; ++binX) {
          const size_t curX = x * factor + binX;
          sum += input->Value(curX, y);
          ++count;
        }
      }
      newImage->SetValue(x, y, sum / (num_t)count);
    }
  }
  return newImage;
}

Image2DPtr ThresholdTools::ShrinkVertically(size_t factor, const Image2D* input,
                                            const Mask2D* mask) {
  const size_t oldHeight = input->Height();
  const size_t newHeight = (oldHeight + factor - 1) / factor;

  Image2DPtr newImage = Image2D::CreateUnsetImagePtr(input->Width(), newHeight);

  for (size_t y = 0; y != newHeight; ++y) {
    size_t avgSize = factor;
    if (avgSize + y * factor > oldHeight) avgSize = oldHeight - y * factor;

    for (size_t x = 0; x != input->Width(); ++x) {
      size_t count = 0;
      num_t sum = 0.0;
      for (size_t binY = 0; binY != avgSize; ++binY) {
        const size_t curY = y * factor + binY;
        if (!mask->Value(x, curY)) {
          sum += input->Value(x, curY);
          ++count;
        }
      }
      if (count == 0) {
        sum = 0.0;
        for (size_t binY = 0; binY != avgSize; ++binY) {
          const size_t curY = y * factor + binY;
          sum += input->Value(x, curY);
          ++count;
        }
      }
      newImage->SetValue(x, y, sum / (num_t)count);
    }
  }
  return newImage;
}

}  // namespace algorithms
