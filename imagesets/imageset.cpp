#include "imageset.h"

#include "bhfitsimageset.h"
#include "coaddedimageset.h"
#include "filterbankset.h"
#include "fitsimageset.h"
#include "h5imageset.h"
#include "msimageset.h"
#include "msstatset.h"
#include "msoptions.h"
#include "parmimageset.h"
#include "pngreader.h"
#include "qualitystatimageset.h"
#include "rfibaselineset.h"
#include "sdhdfimageset.h"

#include <boost/algorithm/string.hpp>

namespace imagesets {
std::unique_ptr<ImageSet> ImageSet::Create(
    const std::vector<std::string>& files, const MSOptions& options) {
  using P = std::unique_ptr<ImageSet>;
  if (files.size() == 1) {
    const std::string& file = files.front();
    if (IsFitsFile(file)) {
      return P(new FitsImageSet(file));
    } else if (IsBHFitsFile(file)) {
      return P(new BHFitsImageSet(file));
    } else if (IsH5File(file)) {
      return P(new H5ImageSet(file));
    } else if (IsRCPRawFile(file)) {
      throw std::runtime_error("Don't know how to open RCP raw files");
    } else if (IsTKPRawFile(file)) {
      throw std::runtime_error("Don't know how to open TKP raw files");
    } else if (IsRawDescFile(file)) {
      throw std::runtime_error("Don't know how to open RCP desc files");
    } else if (IsParmFile(file)) {
      return P(new ParmImageSet(file));
    } else if (IsPngFile(file)) {
      return P(new PngReader(file));
    } else if (IsFilterBankFile(file)) {
      return P(new FilterBankSet(file));
    } else if (IsQualityStatSet(file)) {
      return P(new QualityStatImageSet(file));
    } else if (IsRFIBaselineSet(file)) {
      return P(new RFIBaselineSet(file));
    } else if (IsSdhdfFile(file)) {
      return P(new SdhdfImageSet(file));
    } else {  // it's an MS
      if (options.baselineIntegration.enable.value_or(false))
        return P(new MSStatSet(
            file, options.dataColumnName,
            options.baselineIntegration.mode.value_or(
                BaselineIntegration::Average),
            options.baselineIntegration.differencing.value_or(
                BaselineIntegration::NoDifference),
            options.baselineIntegration.withAutos.value_or(false),
            options.baselineIntegration.withFlagged.value_or(false)));
      else
        return P(new MSImageSet(file, options.ioMode));
    }
  } else {
    return P(new CoaddedImageSet(files, options.ioMode));
  }
}

bool ImageSet::IsH5File(const std::string& file) {
  return (file.size() > 3 &&
          boost::to_upper_copy(file.substr(file.size() - 3)) == ".H5");
}

bool ImageSet::IsBHFitsFile(const std::string& file) {
  const std::string uppFile(boost::to_upper_copy(file));
  return (uppFile.size() > 7 && uppFile.substr(file.size() - 7) == ".BHFITS");
}

bool ImageSet::IsFitsFile(const std::string& file) {
  const std::string uppFile(boost::to_upper_copy(file));
  return (uppFile.size() > 4 && uppFile.substr(file.size() - 4) == ".UVF") ||
         (uppFile.size() > 5 && uppFile.substr(file.size() - 5) == ".FITS") ||
         (uppFile.size() > 7 && uppFile.substr(file.size() - 7) == ".UVFITS") ||
         (uppFile.size() > 7 &&
          uppFile.substr(file.size() - 7) ==
              ".SDFITS");  // Parkes raw files are named like this
}

bool ImageSet::IsRCPRawFile(const std::string& file) {
  return file.size() > 4 && file.substr(file.size() - 4) == ".raw";
}

bool ImageSet::IsTKPRawFile(const std::string& file) {
  return file.size() > 4 && file.substr(file.size() - 4) == ".1ch";
}

bool ImageSet::IsRawDescFile(const std::string& file) {
  return file.size() > 8 && file.substr(file.size() - 8) == ".rawdesc";
}

bool ImageSet::IsParmFile(const std::string& file) {
  return file.size() >= 10 && file.substr(file.size() - 10) == "instrument";
}

bool ImageSet::IsTimeFrequencyStatFile(const std::string& file) {
  return (file.size() >= 24 &&
          file.substr(file.size() - 24) == "counts-timefreq-auto.txt") ||
         (file.size() >= 25 &&
          file.substr(file.size() - 25) == "counts-timefreq-cross.txt");
}

bool ImageSet::IsNoiseStatFile(const std::string& file) {
  return file.find("noise-statistics-tf") != std::string::npos &&
         file.find("txt") != std::string::npos;
}

bool ImageSet::IsPngFile(const std::string& file) {
  return file.size() >= 4 && file.substr(file.size() - 4) == ".png";
}

bool ImageSet::IsFilterBankFile(const std::string& file) {
  return file.size() >= 4 && file.substr(file.size() - 4) == ".fil";
}

bool ImageSet::IsQualityStatSet(const string& file) {
  if (file.empty()) return false;
  std::string copy(file);
  if (*copy.rbegin() == '/') copy.resize(copy.size() - 1);
  std::filesystem::path p(copy);
  return p.filename() == "QUALITY_TIME_STATISTIC";
}

bool ImageSet::IsRFIBaselineSet(const std::string& file) {
  return file.size() >= 6 && file.substr(file.size() - 6) == ".rfibl";
}

bool ImageSet::IsSdhdfFile(const std::string& file) {
  return (file.size() >= 4 && file.substr(file.size() - 4) == ".hdf") ||
         (file.size() >= 6 && file.substr(file.size() - 6) == ".sdhdf");
}

bool ImageSet::IsMSFile(const std::string& file) {
  return (!IsBHFitsFile(file)) && (!IsFitsFile(file)) &&
         (!IsRCPRawFile(file)) && (!IsTKPRawFile(file)) &&
         (!IsRawDescFile(file)) && (!IsParmFile(file)) &&
         (!IsTimeFrequencyStatFile(file)) && (!IsNoiseStatFile(file)) &&
         (!IsPngFile(file)) && (!IsFilterBankFile(file)) &&
         (!IsQualityStatSet(file)) && (!IsRFIBaselineSet(file)) &&
         (!IsSdhdfFile(file));
}
}  // namespace imagesets
