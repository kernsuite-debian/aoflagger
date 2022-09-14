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

  virtual std::unique_ptr<ImageSet> Clone() override { return nullptr; }

  virtual void Initialize() override {}

  virtual std::string Name() const override { return "Png file"; }

  virtual std::vector<std::string> Files() const override {
    return std::vector<std::string>{_path};
  }

  virtual std::string BaselineDescription() override { return Name(); }

  virtual std::string TelescopeName() override {
    return TelescopeFile::TelescopeName(TelescopeFile::GENERIC_TELESCOPE);
  }

  virtual std::unique_ptr<BaselineData> Read(
      class ProgressListener& progress) override;

 private:
  std::string _path;
};

}  // namespace imagesets

#endif
