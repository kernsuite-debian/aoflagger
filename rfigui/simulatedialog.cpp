#include "simulatedialog.h"

#include "../algorithms/testsetgenerator.h"

using algorithms::BackgroundTestSet;
using algorithms::RFITestSet;
using algorithms::TestSetGenerator;

SimulateDialog::SimulateDialog()
    : _nTimesLabel("Number of timesteps:"),
      _nChannelsLabel("Number of channels:"),
      _bandwidthLabel("Bandwidth (MHz):"),
      _polarizationsLabel("Polarizations:"),
      _targetLabel("Target:"),
      _noiseLabel("Noise:"),
      _noiseLevelLabel("Noise level:"),
      _rfiLabel("RFI:") {
  _grid.attach(_nTimesLabel, 1, 0, 1, 1);
  _nTimesEntry.set_text("1000");
  _grid.attach(_nTimesEntry, 2, 0, 1, 1);
  _grid.attach(_nChannelsLabel, 1, 1, 1, 1);
  _nChannelsEntry.set_text("512");
  _grid.attach(_nChannelsEntry, 2, 1, 1, 1);
  _grid.attach(_bandwidthLabel, 1, 2, 1, 1);
  _bandwidthEntry.set_text("32");
  _grid.attach(_bandwidthEntry, 2, 2, 1, 1);

  _grid.attach(_polarizationsLabel, 0, 3, 1, 1);
  _polarizationsSelection.append("Stokes I");
  _polarizationsSelection.append("XX, YY");
  _polarizationsSelection.append("XX, XY, YX, YY");
  _polarizationsSelection.set_active(0);
  _grid.attach(_polarizationsSelection, 1, 3, 2, 1);

  _grid.attach(_targetLabel, 0, 4, 1, 1);
  _targetSelection.append("Current");
  for (BackgroundTestSet bSet = algorithms::BackgroundTestSetFirst();
       bSet != algorithms::BackgroundTestSetLast(); ++bSet) {
    _targetSelection.append(TestSetGenerator::GetDescription(bSet));
  }
  _targetSelection.append(
      TestSetGenerator::GetDescription(algorithms::BackgroundTestSetLast()));
  _targetSelection.set_active(1);
  _grid.attach(_targetSelection, 1, 4, 2, 1);

  _grid.attach(_noiseLabel, 0, 5, 1, 1);
  _noiseSelection.append("None");
  _noiseSelection.append("Complex Gaussian noise");
  _noiseSelection.append("Rayleigh noise");
  _noiseSelection.set_active(1);
  _grid.attach(_noiseSelection, 1, 5, 2, 1);

  _grid.attach(_noiseLevelLabel, 1, 6, 1, 1);
  _noiseLevelEntry.set_text("1");
  _grid.attach(_noiseLevelEntry, 2, 6, 1, 1);

  _grid.attach(_rfiLabel, 0, 7, 1, 1);
  for (RFITestSet rSet = algorithms::RFITestSetFirst();
       rSet != algorithms::RFITestSetLast(); ++rSet) {
    _rfiSelection.append(TestSetGenerator::GetDescription(rSet));
  }
  _rfiSelection.append(
      TestSetGenerator::GetDescription(algorithms::RFITestSetLast()));
  _rfiSelection.set_active(0);
  _grid.attach(_rfiSelection, 1, 7, 2, 1);

  get_content_area()->add(_grid);
  _grid.show_all();

  add_button("Cancel", Gtk::RESPONSE_CANCEL);
  _simulateButton = add_button("Simulate", Gtk::RESPONSE_OK);
}

TimeFrequencyData SimulateDialog::Make() const {
  const size_t width = atoi(_nTimesEntry.get_text().c_str());
  const size_t height = atoi(_nChannelsEntry.get_text().c_str());
  const double noiseLevel = atof(_noiseLevelEntry.get_text().c_str());

  BackgroundTestSet backgroundSet = BackgroundTestSet::Empty;
  const bool replaceBackground = _targetSelection.get_active_row_number() != 0;
  if (replaceBackground)
    backgroundSet =
        BackgroundTestSet(_targetSelection.get_active_row_number() - 1);
  const RFITestSet rfiSet = RFITestSet(_rfiSelection.get_active_row_number());

  TimeFrequencyData data;
  const bool isComplex = _noiseSelection.get_active_row_number() != 2;
  const bool addNoise = _noiseSelection.get_active_row_number() != 0;
  std::vector<aocommon::PolarizationEnum> polarizations;
  if (_polarizationsSelection.get_active_row_number() == 0)
    polarizations = {aocommon::Polarization::StokesI};
  else if (_polarizationsSelection.get_active_row_number() == 1)
    polarizations = {aocommon::Polarization::XX, aocommon::Polarization::YY};
  else
    polarizations = {aocommon::Polarization::XX, aocommon::Polarization::XY,
                     aocommon::Polarization::YX, aocommon::Polarization::YY};
  for (const aocommon::PolarizationEnum pol : polarizations) {
    if (isComplex) {
      if (addNoise) {
        Image2DPtr real = Image2D::MakePtr(
            TestSetGenerator::MakeNoise(width, height, noiseLevel));
        Image2DPtr imag = Image2D::MakePtr(
            TestSetGenerator::MakeNoise(width, height, noiseLevel));
        data = TimeFrequencyData::MakeFromPolarizationCombination(
            data, TimeFrequencyData(pol, real, imag));
      } else {
        Image2DPtr zero = Image2D::CreateZeroImagePtr(width, height);
        data = TimeFrequencyData::MakeFromPolarizationCombination(
            data, TimeFrequencyData(pol, zero, zero));
      }
    } else {
      Image2DPtr amp =
          Image2D::MakePtr(TestSetGenerator::MakeRayleighData(width, height));
      data = TimeFrequencyData::MakeFromPolarizationCombination(
          data, TimeFrequencyData(TimeFrequencyData::AmplitudePart, pol, amp));
    }
  }

  if (replaceBackground) TestSetGenerator::MakeBackground(backgroundSet, data);
  TestSetGenerator::MakeTestSet(rfiSet, data);

  return data;
}
