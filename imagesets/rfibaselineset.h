#ifndef RFI_BASELINE_SET_H
#define RFI_BASELINE_SET_H

#include "../../structures/types.h"

#include "singleimageset.h"

namespace rfiStrategy {

class RFIBaselineSet final : public SingleImageSet {
 public:
  explicit RFIBaselineSet(const std::string& path);

  std::unique_ptr<ImageSet> Clone() override {
    return std::unique_ptr<ImageSet>(new RFIBaselineSet(*this));
  }

  void Initialize() override {}

  std::string Name() const override { return _path; }

  std::string BaselineDescription() override;

  std::vector<std::string> Files() const override {
    return std::vector<std::string>{_path};
  }

  std::string TelescopeName() override { return _telescopeName; }

  std::unique_ptr<BaselineData> Read(class ProgressListener& progress) override;

  void Write(const std::vector<Mask2DCPtr>& masks) override;

 private:
  std::string _path;
  std::string _telescopeName;
  TimeFrequencyData _data;
  TimeFrequencyMetaData _metaData;

  RFIBaselineSet(const RFIBaselineSet& source) = default;
};

}  // namespace rfiStrategy

#endif
