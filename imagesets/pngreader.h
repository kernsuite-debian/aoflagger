#ifndef PNGREADER_H
#define PNGREADER_H

#include <string>
#include <fstream>
#include <set>
#include <map>
#include <cmath>

#include "../structures/types.h"

#include "../lua/telescopefile.h"

#include "singleimageset.h"

#include "../util/logger.h"

namespace imagesets {

class PngReader final : public SingleImageSet {
 public:
  explicit PngReader(const std::string& path) : SingleImageSet(), _path(path) {}

  std::unique_ptr<ImageSet> Clone() override { return nullptr; }

  void Initialize() override {}

  std::string Name() const override { return "Png file"; }

  std::vector<std::string> Files() const override {
    return std::vector<std::string>{_path};
  }

  std::string BaselineDescription() override { return Name(); }

  std::string TelescopeName() override {
    return TelescopeFile::TelescopeName(TelescopeFile::GENERIC_TELESCOPE);
  }

  std::unique_ptr<BaselineData> Read(class ProgressListener& progress) override;

 private:
  std::string _path;
};

}  // namespace imagesets

#endif
