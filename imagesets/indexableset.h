#ifndef INDEXABLE_SET_H
#define INDEXABLE_SET_H

#include "imageset.h"

#include "../msio/baselinereader.h"

#include <memory>
#include <string>

namespace imagesets {

class IndexableSet : public ImageSet {
 public:
  virtual BaselineReaderPtr Reader() const = 0;
  virtual size_t GetAntenna1(const ImageSetIndex& index) const = 0;
  virtual size_t GetAntenna2(const ImageSetIndex& index) const = 0;
  virtual size_t GetBand(const ImageSetIndex& index) const = 0;
  virtual size_t GetField(const ImageSetIndex& index) const = 0;
  virtual size_t GetSequenceId(const ImageSetIndex& index) const = 0;
  virtual size_t AntennaCount() const = 0;
  virtual AntennaInfo GetAntennaInfo(unsigned antennaIndex) const = 0;
  virtual size_t BandCount() const = 0;
  virtual BandInfo GetBandInfo(unsigned bandIndex) const = 0;
  virtual size_t SequenceCount() const = 0;
  virtual std::optional<ImageSetIndex> Index(size_t antenna1, size_t antenna2,
                                             size_t bandIndex,
                                             size_t sequenceId) const = 0;
  virtual FieldInfo GetFieldInfo(unsigned fieldIndex) const = 0;

  std::string TelescopeName() override;

  /**
   * Finds the longest or shortest baseline in the same band/sequence as the
   * input index.
   * @param imageSet Input image set
   * @param index On input, the index that specifies what band/sequence to
   * search. On output, the found index (if found).
   * @param longest true for finding the longest, false for finding the shortest
   * baseline.
   * @returns true if successful
   */
  static bool FindExtremeBaseline(imagesets::ImageSet* imageSet,
                                  imagesets::ImageSetIndex& index,
                                  bool longest);

  /**
   * Finds the baseline with median length in the same band/sequence as the
   * input index.
   * @param imageSet Input image set
   * @param index On input, the index that specifies what band/sequence to
   * search. On output, the found index (if found).
   * @returns true if successful
   */
  static bool FindMedianBaseline(imagesets::ImageSet* imageSet,
                                 imagesets::ImageSetIndex& index);
};

}  // namespace imagesets

inline std::string imagesets::IndexableSet::TelescopeName() {
  auto ms = Reader()->OpenMS();
  return MSMetaData::GetTelescopeName(ms);
}

#endif
