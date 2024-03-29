#include <stdexcept>
#include <set>
#include <sstream>

#include "rspreader.h"
#include "../structures/image2d.h"
#include "../structures/mask2d.h"
#include "../structures/samplerow.h"

#include "../util/logger.h"
#include "../util/ffttools.h"

const unsigned char RSPReader::BitReverseTable256[256] = {
#define R2(n) n, n + 2 * 64, n + 1 * 64, n + 3 * 64
#define R4(n) R2(n), R2(n + 2 * 16), R2(n + 1 * 16), R2(n + 3 * 16)
#define R6(n) R4(n), R4(n + 2 * 4), R4(n + 1 * 4), R4(n + 3 * 4)
    R6(0), R6(2), R6(1), R6(3)};

const unsigned long RSPReader::STATION_INTEGRATION_STEPS = 1024;

const unsigned int RSPReader::RCPBeamletData::SIZE = 8;

const unsigned int RSPReader::RCPApplicationHeader::SIZE = 16;

std::pair<TimeFrequencyData, TimeFrequencyMetaDataPtr>
RSPReader::ReadChannelBeamlet(unsigned long timestepStart,
                              unsigned long timestepEnd, unsigned beamletCount,
                              unsigned beamletIndex) {
  const unsigned width = timestepEnd - timestepStart;

  std::pair<TimeFrequencyData, TimeFrequencyMetaDataPtr> data =
      ReadSingleBeamlet(timestepStart * (unsigned long)256,
                        timestepEnd * (unsigned long)256, beamletCount,
                        beamletIndex);

  const TimeFrequencyData allX = data.first.Make(aocommon::Polarization::XX);
  const TimeFrequencyData allY = data.first.Make(aocommon::Polarization::YY);
  const Image2DCPtr xr = allX.GetRealPart();
  const Image2DCPtr xi = allX.GetImaginaryPart();
  const Image2DCPtr yr = allY.GetRealPart();
  const Image2DCPtr yi = allY.GetImaginaryPart();
  const Mask2DCPtr mask = data.first.GetSingleMask();

  Image2DPtr outXR = Image2D::CreateUnsetImagePtr(width, 256),
             outXI = Image2D::CreateUnsetImagePtr(width, 256),
             outYR = Image2D::CreateUnsetImagePtr(width, 256),
             outYI = Image2D::CreateUnsetImagePtr(width, 256);
  const Mask2DPtr outMask = Mask2D::CreateUnsetMaskPtr(width, 256);

  std::vector<double> observationTimes;
  for (unsigned long timestep = 0; timestep < timestepEnd - timestepStart;
       ++timestep) {
    const unsigned long timestepIndex = timestep * 256;
    SampleRow realX = SampleRow::MakeFromRow(xr.get(), timestepIndex, 256, 0),
              imaginaryX =
                  SampleRow::MakeFromRow(xi.get(), timestepIndex, 256, 0),
              realY = SampleRow::MakeFromRow(yr.get(), timestepIndex, 256, 0),
              imaginaryY =
                  SampleRow::MakeFromRow(yi.get(), timestepIndex, 256, 0);

    FFTTools::FFT(realX, imaginaryX);
    FFTTools::FFT(realY, imaginaryY);

    realX.SetVerticalImageValues(outXR.get(), timestep);
    imaginaryX.SetVerticalImageValues(outXI.get(), timestep);
    realY.SetVerticalImageValues(outYR.get(), timestep);
    imaginaryY.SetVerticalImageValues(outYI.get(), timestep);

    observationTimes.push_back(
        data.second->ObservationTimes()[timestepIndex + 256 / 2]);

    size_t validValues = 0;
    for (unsigned y = 0; y < 256; ++y) {
      if (!mask->Value(timestepIndex + y, 0)) ++validValues;
    }
    for (unsigned y = 0; y < 256; ++y) {
      outMask->SetValue(timestep, y, validValues == 0);
    }
  }

  data.first = TimeFrequencyData(aocommon::Polarization::XX, outXR, outXI,
                                 aocommon::Polarization::YY, outYR, outYI);
  data.first.SetGlobalMask(outMask);
  BandInfo band = data.second->Band();
  band.channels.clear();
  for (unsigned i = 0; i < 256; ++i) {
    ChannelInfo channel;
    channel.frequencyHz = i + 1;
    channel.frequencyIndex = i;
    band.channels.push_back(channel);
  }
  data.second->SetBand(band);
  data.second->SetObservationTimes(observationTimes);
  return data;
}

std::pair<TimeFrequencyData, TimeFrequencyMetaDataPtr>
RSPReader::ReadSingleBeamlet(unsigned long timestepStart,
                             unsigned long timestepEnd, unsigned beamletCount,
                             unsigned beamletIndex) {
  std::pair<TimeFrequencyData, TimeFrequencyMetaDataPtr> data =
      ReadAllBeamlets(timestepStart, timestepEnd, beamletCount);

  const unsigned width = timestepEnd - timestepStart;
  const Image2DPtr realX = Image2D::CreateZeroImagePtr(width, 1);
  const Image2DPtr imaginaryX = Image2D::CreateZeroImagePtr(width, 1);
  const Image2DPtr realY = Image2D::CreateZeroImagePtr(width, 1);
  const Image2DPtr imaginaryY = Image2D::CreateZeroImagePtr(width, 1);
  const Mask2DPtr mask = Mask2D::CreateUnsetMaskPtr(width, 1);

  const TimeFrequencyData allX = data.first.Make(aocommon::Polarization::XX);
  const TimeFrequencyData allY = data.first.Make(aocommon::Polarization::YY);
  const Image2DCPtr xr = allX.GetRealPart();
  const Image2DCPtr xi = allX.GetImaginaryPart();
  const Image2DCPtr yr = allY.GetRealPart();
  const Image2DCPtr yi = allY.GetImaginaryPart();
  const Mask2DCPtr maskWithBeamlets = data.first.GetSingleMask();

  for (unsigned x = 0; x < width; ++x) {
    realX->SetValue(x, 0, xr->Value(x, beamletIndex));
    imaginaryX->SetValue(x, 0, xi->Value(x, beamletIndex));
    realY->SetValue(x, 0, yr->Value(x, beamletIndex));
    imaginaryY->SetValue(x, 0, yi->Value(x, beamletIndex));
    mask->SetValue(x, 0, maskWithBeamlets->Value(x, beamletIndex));
  }
  data.first = TimeFrequencyData(aocommon::Polarization::XX, realX, imaginaryX,
                                 aocommon::Polarization::YY, realY, imaginaryY);
  data.first.SetGlobalMask(mask);
  BandInfo band = data.second->Band();
  band.channels[0] = data.second->Band().channels[beamletIndex];
  band.channels.resize(1);
  data.second->SetBand(band);
  return data;
}

unsigned long RSPReader::TimeStepCount(size_t beamletCount) const {
  std::ifstream stream(_rawFile.c_str(),
                       std::ios_base::binary | std::ios_base::in);
  stream.seekg(0, std::ios_base::end);
  const unsigned long fileSize = stream.tellg();

  stream.seekg(0, std::ios_base::beg);
  RCPApplicationHeader firstHeader;
  firstHeader.Read(stream);
  const unsigned long bytesPerFrame =
      beamletCount * firstHeader.nofBlocks * RCPBeamletData::SIZE +
      RCPApplicationHeader::SIZE;
  const unsigned long frames = fileSize / bytesPerFrame;

  Logger::Debug << "File has " << frames << " number of frames ("
                << ((double)(frames * firstHeader.nofBlocks *
                             STATION_INTEGRATION_STEPS) /
                    _clockSpeed)
                << "s of data)\n";

  return frames * firstHeader.nofBlocks;
}

std::pair<TimeFrequencyData, TimeFrequencyMetaDataPtr>
RSPReader::ReadAllBeamlets(unsigned long timestepStart,
                           unsigned long timestepEnd, unsigned beamletCount) {
  const unsigned width = timestepEnd - timestepStart;
  const Image2DPtr realX = Image2D::CreateZeroImagePtr(width, beamletCount);
  const Image2DPtr imaginaryX =
      Image2D::CreateZeroImagePtr(width, beamletCount);
  const Image2DPtr realY = Image2D::CreateZeroImagePtr(width, beamletCount);
  const Image2DPtr imaginaryY =
      Image2D::CreateZeroImagePtr(width, beamletCount);
  const Mask2DPtr mask = Mask2D::CreateSetMaskPtr<true>(width, beamletCount);

  std::ifstream file(_rawFile.c_str(),
                     std::ios_base::binary | std::ios_base::in);
  size_t frame = 0;
  std::set<short> stations;

  const TimeFrequencyMetaDataPtr metaData =
      TimeFrequencyMetaDataPtr(new TimeFrequencyMetaData());
  BandInfo band;
  for (size_t i = 0; i < beamletCount; ++i) {
    ChannelInfo channel;
    channel.frequencyHz = i + 1;
    channel.frequencyIndex = i;
    band.channels.push_back(channel);
  }
  metaData->SetBand(band);

  std::vector<double> observationTimes;

  // Read a header and determine the reading start position
  // Because timestepStart might fall within a block, the
  RCPApplicationHeader firstHeader;
  firstHeader.Read(file);
  const unsigned long bytesPerFrame =
      beamletCount * firstHeader.nofBlocks * RCPBeamletData::SIZE +
      RCPApplicationHeader::SIZE;
  const unsigned long startFrame =
      timestepStart / (unsigned long)firstHeader.nofBlocks;
  const unsigned long startByte = startFrame * bytesPerFrame;
  const unsigned long offsetFromStart =
      timestepStart - (startFrame * firstHeader.nofBlocks);
  // Logger::Debug << "Seeking to " << startByte << " (timestepStart=" <<
  // timestepStart << ", offsetFromStart=" << offsetFromStart << ", startFrame="
  // << startFrame << ",bytesPerFrame=" << bytesPerFrame << ")\n";
  file.seekg(startByte, std::ios_base::beg);

  // Read the frames
  unsigned long x = 0;
  while (x < width + offsetFromStart && file.good()) {
    RCPApplicationHeader header;
    header.Read(file);
    if (header.versionId != 2) {
      std::stringstream s;
      s << "Corrupted header found in frame " << frame << "!";
      throw std::runtime_error(s.str());
    }
    if (stations.count(header.stationId) == 0) {
      stations.insert(header.stationId);
      AntennaInfo antenna;
      std::stringstream s;
      s << "LOFAR station with index " << header.stationId;
      antenna.name = s.str();
      metaData->SetAntenna1(antenna);
      metaData->SetAntenna2(antenna);
    }
    for (size_t j = 0; j < beamletCount; ++j) {
      for (size_t i = 0; i < header.nofBlocks; ++i) {
        RCPBeamletData data;
        data.Read(file);
        if (i + x < width + offsetFromStart && i + x >= offsetFromStart) {
          const unsigned long pos = i + x - offsetFromStart;
          realX->SetValue(pos, j, data.xr);
          imaginaryX->SetValue(pos, j, data.xi);
          realY->SetValue(pos, j, data.yr);
          imaginaryY->SetValue(pos, j, data.yi);
          mask->SetValue(pos, j, false);
        }
      }
    }
    x += header.nofBlocks;
    ++frame;
  }
  // Logger::Debug << "Read " << frame << " frames.\n";

  for (unsigned long i = 0; i < width; ++i) {
    const unsigned long pos = i + timestepStart;
    const double time =
        (double)pos * (double)STATION_INTEGRATION_STEPS / (double)_clockSpeed;
    observationTimes.push_back(time);
  }

  metaData->SetObservationTimes(observationTimes);

  std::pair<TimeFrequencyData, TimeFrequencyMetaDataPtr> data;
  data.first = TimeFrequencyData(aocommon::Polarization::XX, realX, imaginaryX,
                                 aocommon::Polarization::YY, realY, imaginaryY);
  data.first.SetGlobalMask(mask);
  data.second = metaData;
  return data;
}

void RSPReader::ReadForStatistics(unsigned beamletCount) {
  const long unsigned timesteps = TimeStepCount(beamletCount);
  const long unsigned stepSize = 1024;
  std::vector<BeamletStatistics> statistics(beamletCount),
      timeStartStatistics(beamletCount);

  std::vector<std::ofstream*> statFile(beamletCount);
  for (unsigned i = 0; i < beamletCount; ++i) {
    std::ostringstream str;
    str << "rsp-statistics" << i << ".txt";
    statFile[i] = new std::ofstream(str.str().c_str());
  }

  double startTime = -1.0, periodStartTime = -1.0;

  for (unsigned long timestepIndex = 0; timestepIndex < timesteps;
       timestepIndex += stepSize) {
    // Read the data
    unsigned long end = timestepIndex + stepSize;
    if (end > timesteps) end = timesteps;
    const std::pair<TimeFrequencyData, TimeFrequencyMetaDataPtr> dataPair =
        ReadAllBeamlets(timestepIndex, end, beamletCount);
    const TimeFrequencyData& data = dataPair.first;
    if (startTime == -1.0) {
      startTime = dataPair.second->ObservationTimes()[0];
      periodStartTime = startTime;
    }

    // Count the statistics
    for (unsigned imageIndex = 0; imageIndex < data.ImageCount();
         ++imageIndex) {
      const Image2DCPtr image = data.GetImage(imageIndex);
      for (unsigned y = 0; y < image->Height(); ++y) {
        for (unsigned x = 0; x < image->Width(); ++x) {
          int value = (int)image->Value(x, y);
          if (value < 0) {
            value = -value;
            ++(statistics[y].bitUseCount[15]);
          }
          unsigned highestBit = (value != 0) ? 1 : 0;
          for (unsigned bit = 0; bit < 15; ++bit) {
            if ((value & (2 << bit)) != 0) {
              highestBit = bit + 1;
            }
          }
          for (unsigned bit = 0; bit < highestBit; ++bit)
            ++(statistics[y].bitUseCount[bit]);
          ++(statistics[y].totalCount);
        }
      }
    }
    if ((timestepIndex / stepSize) % 100000 == 0 ||
        timestepIndex + stepSize >= timesteps) {
      for (unsigned i = 0; i < beamletCount; ++i) {
        Logger::Info << "Beamlet index " << i << ":\n";
        statistics[i].Print();
      }
    }
    if ((dataPair.second->ObservationTimes()[0] - periodStartTime) > 60.0) {
      Logger::Debug << "Processed 1 minute of data ("
                    << (dataPair.second->ObservationTimes()[0] - startTime)
                    << "s)\n";
      for (unsigned i = 0; i < beamletCount; ++i) {
        (*statFile[i]) << (periodStartTime - startTime) << '\t'
                       << (statistics[i].totalCount -
                           timeStartStatistics[i].totalCount);
        statistics[i].totalCount = timeStartStatistics[i].totalCount;
        for (unsigned bit = 0; bit < 15; ++bit) {
          (*statFile[i]) << '\t'
                         << (statistics[i].bitUseCount[bit] -
                             timeStartStatistics[i].bitUseCount[bit]);
          timeStartStatistics[i].bitUseCount[bit] =
              statistics[i].bitUseCount[bit];
        }
        (*statFile[i]) << '\n';
      }

      periodStartTime = dataPair.second->ObservationTimes()[0];
    }
  }

  for (unsigned i = 0; i < beamletCount; ++i) {
    Logger::Info << "Beamlet index " << i << ":\n";
    statistics[i].Print();
    delete statFile[i];
  }
}
