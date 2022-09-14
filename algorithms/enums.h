#ifndef ALGORITHMS_ENUMS_H
#define ALGORITHMS_ENUMS_H

namespace algorithms {

enum class RFITestSet {
  Empty,
  SpectralLines,
  GaussianSpectralLines,
  IntermittentSpectralLines,
  FullBandBursts,
  HalfBandBursts,
  VaryingBursts,
  GaussianBursts,
  SinusoidalBursts,
  SlewedGaussians,
  FluctuatingBursts,
  StrongPowerLaw,
  MediumPowerLaw,
  WeakPowerLaw,
  PolarizedSpike
};
constexpr inline RFITestSet RFITestSetFirst() { return RFITestSet::Empty; }
constexpr inline RFITestSet RFITestSetLast() {
  return RFITestSet::PolarizedSpike;
}
RFITestSet inline operator++(RFITestSet& x) {
  return x = (RFITestSet)(std::underlying_type<RFITestSet>::type(x) + 1);
}
enum class BackgroundTestSet {
  Empty,
  LowFrequency,
  HighFrequency,
  ThreeSources,
  FiveSources,
  FiveFilteredSources,
  StaticSidelobeSource,
  StrongVariableSidelobeSource,
  FaintVariableSidelobeSource,
  Checker
};
constexpr inline BackgroundTestSet BackgroundTestSetFirst() {
  return BackgroundTestSet::Empty;
}
constexpr inline BackgroundTestSet BackgroundTestSetLast() {
  return BackgroundTestSet::Checker;
}
BackgroundTestSet inline operator++(BackgroundTestSet& x) {
  return x = (BackgroundTestSet)(std::underlying_type<BackgroundTestSet>::type(
                                     x) +
                                 1);
}

}  // namespace algorithms

#endif
