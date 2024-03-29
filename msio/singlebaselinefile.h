#ifndef SINGLE_BASELINE_FILE_H
#define SINGLE_BASELINE_FILE_H

#include <istream>
#include <ostream>

#include "../structures/timefrequencydata.h"
#include "../structures/timefrequencymetadata.h"

#include "../util/progress/progresslistener.h"

class SingleBaselineFile {
 public:
  void Read(std::istream& stream, ProgressListener& progress);
  void Write(std::ostream& stream);

  static void Serialize(std::ostream& stream, const TimeFrequencyData& data);
  static void Serialize(std::ostream& stream,
                        const TimeFrequencyMetaData& metaData);
  static void Serialize(std::ostream& stream, const Image2D& image);
  static void Serialize(std::ostream& stream, const Mask2D& mask);

  static TimeFrequencyData UnserializeTFData(std::istream& stream,
                                             ProgressListener& progress);
  static TimeFrequencyMetaData UnserializeMetaData(std::istream& stream);
  static Image2D UnserializeImage(std::istream& stream,
                                  ProgressListener& progress,
                                  size_t progressOffset, size_t progressMax);
  static Mask2D UnserializeMask(std::istream& stream);

  TimeFrequencyData data;
  TimeFrequencyMetaData metaData;
  std::string telescopeName;
};

#endif
