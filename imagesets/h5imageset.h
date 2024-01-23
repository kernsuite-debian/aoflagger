#ifndef IMAGESETS_H5_IMAGE_SET_H_
#define IMAGESETS_H5_IMAGE_SET_H_

#include <algorithm>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "imageset.h"

class ProgressListener;

namespace imagesets {

/**
 * This class provides the support for the LOFAR beam-formed file format.
 * There's a tutorial on this format made by Cees Bassa:
 *
 * - https://github.com/cbassa/lofar_bf_tutorials/tree/master/solutions
 *
 * He summarized the most important bits (to read the axes & data) as (in Python
 * code):
 * @code{.py}
 * stokes = h5["/SUB_ARRAY_POINTING_000/BEAM_000/STOKES_0"]
 * freq =
 * h5["/SUB_ARRAY_POINTING_000/BEAM_000/COORDINATES/COORDINATE_1"].attrs["AXIS_VALUES_WORLD"]
 * * 1e-6 tsamp =
 * h5["/SUB_ARRAY_POINTING_000/BEAM_000/COORDINATES/COORDINATE_0"].attrs["INCREMENT"]
 * t = tsamp * np.arange(stokes.shape[0])
 * @endcode
 *
 * This is more or less what this class does. The 'stokes' data needs to be
 * transposed from frequency first to time first. The attribute 'TELESCOPE' is
 * read for determining the telescope, but as far as we know only LOFAR uses
 * this format.
 */
class H5ImageSet final : public ImageSet {
 public:
  explicit H5ImageSet(const std::string& path);
  H5ImageSet(const H5ImageSet& source)
      : path_(source.path_),
        telescope_name_(source.telescope_name_),
        interval_start_(source.interval_start_),
        interval_end_(source.interval_end_),
        requests_(),        // Ongoing requests should not be copied
        baseline_buffer_()  // Already read data should not be copied
  {}
  H5ImageSet& operator=(const H5ImageSet& rhs) = delete;

  size_t Size() const override { return 1; }
  std::string Description(const ImageSetIndex& index) const override;
  std::unique_ptr<ImageSet> Clone() override {
    return std::make_unique<H5ImageSet>(*this);
  }
  void Initialize() override;

  std::string Name() const override { return "H5/raw"; }

  std::vector<std::string> Files() const override {
    return std::vector<std::string>{path_};
  }

  std::string TelescopeName() override { return telescope_name_; }

  void SetInterval(std::optional<size_t> start, std::optional<size_t> end) {
    interval_start_ = start;
    interval_end_ = end;
  }

  bool HasCrossCorrelations() const override { return false; }

  void AddReadRequest(const ImageSetIndex& index) override {
    requests_.emplace_back(index);
  }
  void PerformReadRequests(ProgressListener& progress) override;

  std::unique_ptr<BaselineData> GetNextRequested() override {
    std::unique_ptr<BaselineData> baseline(std::move(baseline_buffer_.front()));
    baseline_buffer_.pop();
    return baseline;
  }

  void AddWriteFlagsTask(const ImageSetIndex& index,
                         std::vector<Mask2DCPtr>& flags) override;
  void PerformWriteFlagsTask() override {}

 private:
  std::string path_;
  std::string telescope_name_;
  std::optional<size_t> interval_start_;
  std::optional<size_t> interval_end_;
  std::vector<ImageSetIndex> requests_;
  std::queue<std::unique_ptr<BaselineData>> baseline_buffer_;

  std::unique_ptr<BaselineData> LoadData(ProgressListener& progress,
                                         const ImageSetIndex& index);
};
}  // namespace imagesets

#endif
