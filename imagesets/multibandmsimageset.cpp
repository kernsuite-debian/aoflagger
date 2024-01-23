
#include "multibandmsimageset.h"

#include "../msio/directbaselinereader.h"
#include "../msio/memorybaselinereader.h"
#include "../msio/reorderingbaselinereader.h"
#include "../util/logger.h"
#include "../util/progress/dummyprogresslistener.h"
#include "../util/progress/subtasklistener.h"
#include "../util/stopwatch.h"
#include "joinedspwset.h"

#include <aocommon/parallelfor.h>

#include <limits>
#include <numeric>
#include <utility>

namespace imagesets {

const size_t MultiBandMsImageSet::kNotFound =
    std::numeric_limits<size_t>::max();

static std::unique_ptr<BaselineReader> CreateReader(const std::string& ms_name,
                                                    BaselineIOMode io_mode) {
  switch (io_mode) {
    case BaselineIOMode::DirectReadMode:
      return std::make_unique<DirectBaselineReader>(ms_name);

    case BaselineIOMode::ReorderingReadMode:
      return std::make_unique<ReorderingBaselineReader>(ms_name);

    case BaselineIOMode::AutoReadMode:
    case BaselineIOMode::MemoryReadMode:
      return std::make_unique<MemoryBaselineReader>(ms_name);
  }
  return nullptr;
}

MultiBandMsImageSet::MultiBandMsImageSet(
    const std::vector<std::string>& ms_names, BaselineIOMode io_mode,
    std::optional<size_t> start_time_step, std::optional<size_t> end_time_step,
    size_t n_threads)
    : ms_names_(ms_names) {
  // AutoReadMode behaves as-if MemoryReadMode. When the estimated amount of
  // memory is insufficent switch to the direct reader. This behaviour matches
  // MSImageSet::initReader.
  if (io_mode == BaselineIOMode::AutoReadMode &&
      !MemoryBaselineReader::IsEnoughMemoryAvailable(
          ms_names.size() * BaselineReader::MeasurementSetIntervalDataSize(
                                ms_names[0], start_time_step, end_time_step)))
    io_mode = BaselineIOMode::ReorderingReadMode;

  for (const std::string& ms_name : ms_names_) {
    readers_.emplace_back(CreateReader(ms_name, io_mode));
    readers_.back()->SetInterval(start_time_step, end_time_step);
  }

  assert(n_threads != 0 && n_threads <= readers_.size() &&
         "Caller should provide a valid number of execution threads.");
  ReadData(n_threads);
  ProcessMetaData();
}

static std::vector<BaselineReader*> GetWriters(
    std::vector<std::unique_ptr<BaselineReader>>& readers) {
  std::vector<BaselineReader*> writers;
  // Only the modified readers need to be written.
  for (std::unique_ptr<BaselineReader>& reader : readers)
    if (reader->IsModified()) writers.emplace_back(reader.get());

  return writers;
}

void MultiBandMsImageSet::WriteToMs(size_t n_threads) {
  assert(n_threads != 0 && n_threads <= readers_.size() &&
         "Caller should provide a valid number of execution threads.");
  const Stopwatch watch(true);

  const std::vector<BaselineReader*> writers = GetWriters(readers_);
  aocommon::ParallelFor<size_t> executor(n_threads);
  executor.Run(0, writers.size(), [&](size_t i) { writers[i]->WriteToMs(); });

  Logger::Debug << "Writing took " << watch.ToString() << ".\n";
}

std::optional<ImageSetIndex> MultiBandMsImageSet::Index(
    size_t antenna_1, size_t antenna_2, size_t band, size_t sequence_id) const {
  const size_t value =
      FindBaselineIndex(antenna_1, antenna_2, band, sequence_id);
  if (value != kNotFound) {
    return ImageSetIndex(Size(), value);
  }

  return {};
}

static std::pair<BandInfo, std::vector<size_t>> CombineBands(
    const std::vector<const MSMetaData*>& meta_data) {
  assert(meta_data[0]->BandCount() == 1 &&
         "The reader should have validated the number of bands");

  BandInfo band_info = meta_data[0]->GetBandInfo(0);
  std::vector<ChannelInfo>& channels = band_info.channels;
  std::vector<size_t> channels_per_band;
  channels_per_band.push_back(channels.size());

  std::for_each(
      meta_data.begin() + 1, meta_data.end(), [&](const MSMetaData* element) {
        assert(element->BandCount() == 1 &&
               "The reader should have validated the number of bands");

        const BandInfo& band = element->GetBandInfo(0);
        std::copy(band.channels.begin(), band.channels.end(),
                  std::back_inserter(channels));

        channels_per_band.push_back(band.channels.size());
      });

  return {std::move(band_info), std::move(channels_per_band)};
}

static const std::vector<std::pair<size_t, size_t>>& GetBaselines(
    const MSMetaData* meta_data) {
  return meta_data->GetBaselines();
}

static const std::set<double>& GetObservationTimes(
    const MSMetaData* meta_data) {
  return meta_data->GetObservationTimes();
}

static const std::vector<std::set<double>>& GetObservationTimesPerSequence(
    const MSMetaData* meta_data) {
  return meta_data->GetObservationTimesPerSequence();
}

static const std::vector<AntennaInfo>& GetAntennae(
    const MSMetaData* meta_data) {
  return meta_data->GetAntennas();
}

static const std::vector<FieldInfo>& GetFields(const MSMetaData* meta_data) {
  return meta_data->GetFields();
}

static const std::vector<MSMetaData::Sequence>& GetSequences(
    const MSMetaData* meta_data) {
  return meta_data->GetSequences();
}

template <class Type, class Functor>
static void ValidateEqual(const Type& lhs,
                          std::vector<const MSMetaData*>::const_iterator first,
                          std::vector<const MSMetaData*>::const_iterator last,
                          Functor&& functor, const std::string& description) {
  if (!std::all_of(first, last, [&](const MSMetaData* element) {
        return lhs == functor(element);
      })) {
    throw std::runtime_error(
        "The loaded measurement sets are not compatible in this dimension: " +
        description);
  }
}

template <class Functor>
static auto ExtractField(const std::vector<const MSMetaData*>& meta_data,
                         Functor&& functor, const std::string& description) {
  assert(!meta_data.empty());
  auto result = functor(meta_data[0]);
  ValidateEqual(result, meta_data.begin() + 1, meta_data.end(),
                std::forward<Functor>(functor), description);
  return result;
}

void MultiBandMsImageSet::ReadData(size_t n_threads) {
  const Stopwatch watch(true);
  aocommon::ParallelFor<size_t> executor(n_threads);
  executor.Run(0, readers_.size(), [&](size_t i) {
    readers_[i]->PrepareReadWrite(BaselineReader::dummy_progress_);
  });
  Logger::Debug << "Reading took " << watch.ToString() << ".\n";
}

// Returns the metadata of the readers and initializes their main tables.
static std::vector<const MSMetaData*> GetInitializedMetaData(
    std::vector<std::unique_ptr<BaselineReader>>::iterator first,
    std::vector<std::unique_ptr<BaselineReader>>::iterator last) {
  std::vector<const MSMetaData*> result;
  std::transform(first, last, std::back_inserter(result),
                 [](std::unique_ptr<BaselineReader>& reader) {
                   MSMetaData& meta_data{reader->MetaData()};
                   meta_data.InitializeMainTableData();
                   return &meta_data;
                 });
  return result;
}

namespace {
// These two helper functions below are based on
// https://stackoverflow.com/questions/17074324/how-can-i-sort-two-vectors-in-the-same-way-with-criteria-that-uses-only-one-of

// They are used in MultiBandMsImageSet::ProcessMetaData() to sort multiple
// vectors simulatenously by frequency. There is a nicer ranges based solution,
// supported from C++23 onwards, see the comment where the functions are used.
// The functions here can be removed when moving to to ranges based solution

/**
 * Returns an indexing vector that sorts vector \p input according to comparator
 * \p compare
 */
template <typename T, typename Compare>
std::vector<std::size_t> MakeSortingPermutation(const std::vector<T>& input,
                                                const Compare& compare) {
  std::vector<std::size_t> permutation(input.size());
  std::iota(permutation.begin(), permutation.end(), 0);
  std::sort(permutation.begin(), permutation.end(),
            [&](std::size_t i, std::size_t j) {
              return compare(input[i], input[j]);
            });
  return permutation;
}

/**
 * Permute the vector \p input using indexing vector \p permutation.
 */
template <typename T>
void ApplyPermutation(std::vector<T>& input,
                      const std::vector<std::size_t>& permutation) {
  std::vector<bool> done(input.size());
  for (std::size_t i = 0; i < input.size(); ++i) {
    if (done[i]) {
      continue;
    }
    done[i] = true;
    std::size_t prev_j = i;
    std::size_t j = permutation[i];
    while (i != j) {
      std::swap(input[prev_j], input[j]);
      done[j] = true;
      prev_j = j;
      j = permutation[j];
    }
  }
}

/**
 * Find the order in which the channels in the spws are sorted.
 *
 * Returns std::optional<bool>(true) when all spws are in ascending frequency
 * order, std::optiona<bool>(false) when in descending frequency order.
 * std::optional<bool>() when in mixed order
 */
std::optional<bool> IsInAscendingFrequencyOrder(
    const std::vector<const MSMetaData*>& meta_data) {
  std::optional<bool> frequency_is_ascending;
  for (const MSMetaData* ms_meta_data : meta_data) {
    // Only check the first band, assuming the rest is in the same order
    const std::vector<ChannelInfo>& channels =
        ms_meta_data->GetBandInfo(0).channels;
    // If there is only one channel, there is no ordering
    if (channels.size() == 1) continue;
    // Determine the ordering of this ms from the order of the first twp
    // channels
    bool ms_frequency_is_ascending =
        channels[0].frequencyHz < channels[1].frequencyHz;
    if (frequency_is_ascending.has_value()) {
      // If an ordering has already been established, the current ms should be
      // conformal
      if (*frequency_is_ascending != ms_frequency_is_ascending) {
        return std::optional<bool>();
      }
    } else {
      // There was no ordering established, the current ms determines the
      // ordering
      frequency_is_ascending = ms_frequency_is_ascending;
    }
  }
  // No ordering has been established (all ms have a single channel)
  // Set the ordering arbitrarily to ascending
  if (!frequency_is_ascending.has_value()) frequency_is_ascending = true;

  return frequency_is_ascending;
}

}  // anonymous namespace

void MultiBandMsImageSet::ProcessMetaData() {
  std::vector<const MSMetaData*> meta_data =
      GetInitializedMetaData(readers_.begin(), readers_.end());

  // The ms's should be combined in frequency order
  // Either ascending or descending depending on the order of the channels in
  // the ms
  std::optional<bool> frequency_is_ascending =
      IsInAscendingFrequencyOrder(meta_data);
  if (!frequency_is_ascending.has_value()) {
    throw std::runtime_error(
        "Trying to concatenate MeasurementSets that have not the same "
        "ordering in frequency.");
  }

  // In C++23 multiple vectors can be sorted simultaneously using ranges:
  //
  //   std::ranges::sort(std::views::zip(ms_names_, readers_, meta_data),
  //   [frequency_is_ascending](auto&& a, auto&& b) {
  //     return (std::get<2>(a)->GetBandInfo(0).channels[0].frequencyHz <
  //             std::get<2>(b)->GetBandInfo(0).channels[0].frequencyHz) !=
  //             !(*frequency_is_ascending);
  //   });
  //
  // Without ranges support, the helper functions MakeSortingPermutation and
  // ApplyPermutation are needed

  // Get the permutation to sort the ms's in ascending/descending frequency
  // order The != operator is equivalent to a xor operation, toggling the
  // comparison
  std::vector<size_t> permutation = MakeSortingPermutation(
      meta_data,
      [frequency_is_ascending](const MSMetaData* a, const MSMetaData* b) {
        return (a->GetBandInfo(0).channels[0].frequencyHz <
                b->GetBandInfo(0).channels[0].frequencyHz) !=
               !(*frequency_is_ascending);
      });

  // Apply the permutation to ms_names_ and its derivatives
  ApplyPermutation(ms_names_, permutation);
  ApplyPermutation(readers_, permutation);
  ApplyPermutation(meta_data, permutation);

  // These fields are only validated.
  ExtractField(meta_data, GetBaselines, "baselines");
  ExtractField(meta_data, GetObservationTimes, "timesteps");

  // These fields are validated and cached.
  antennae_ = ExtractField(meta_data, GetAntennae, "antennas");
  fields_ = ExtractField(meta_data, GetFields, "fields");
  sequences_ = ExtractField(meta_data, GetSequences, "sequences");
  observation_times_per_sequence_ = ExtractField(
      meta_data, GetObservationTimesPerSequence, "timesteps per sequence");
  std::tie(band_, channels_per_band_) = CombineBands(meta_data);
}

size_t MultiBandMsImageSet::FindBaselineIndex(size_t antenna_1,
                                              size_t antenna_2,
                                              size_t /* band */,
                                              size_t sequence_id) const {
  // TODO This is a linear search, when it becomes measureable it should be
  // improved.
  size_t index = 0;
  for (const MSMetaData::Sequence& sequence : sequences_) {
    const bool antennaMatch =
        (sequence.antenna1 == antenna_1 && sequence.antenna2 == antenna_2) ||
        (sequence.antenna1 == antenna_2 && sequence.antenna2 == antenna_1);

    if (antennaMatch && sequence.sequenceId == sequence_id) {
      return index;
    }
    ++index;
  }

  return kNotFound;
}

std::string MultiBandMsImageSet::Description(const ImageSetIndex& index) const {
  std::stringstream sstream;
  const MSMetaData::Sequence& sequence = sequences_[GetSequenceIndex(index)];
  const AntennaInfo& antenna_1 = antennae_[sequence.antenna1];
  const AntennaInfo& antenna_2 = antennae_[sequence.antenna2];

  sstream << antenna_1.station << ' ' << antenna_1.name << " x "
          << antenna_2.station << ' ' << antenna_2.name;

  const double band_start =
      round(band_.channels.front().frequencyHz / 100000.0) / 10.0;
  const double band_end =
      round(band_.channels.back().frequencyHz / 100000.0) / 10.0;
  sstream << ", spectrally concatenated (" << band_start << "MHz -" << band_end
          << "MHz)";

  if (SequenceCount() > 1) {
    sstream << ", seq " << sequence.sequenceId;
  }
  return sstream.str();
}

static TimeFrequencyMetaDataCPtr GetMetaData(
    BaselineReader& reader, const MSMetaData::Sequence& sequence,
    const std::vector<UVW>& uvw) {
  auto result = std::make_unique<TimeFrequencyMetaData>();

  MSMetaData& meta_data = reader.MetaData();
  result->SetAntenna1(meta_data.GetAntennaInfo(sequence.antenna1));
  result->SetAntenna2(meta_data.GetAntennaInfo(sequence.antenna2));
  result->SetBand(meta_data.GetBandInfo(0));
  result->SetField(meta_data.GetFieldInfo(sequence.fieldId));
  const std::set<double>& observation_times =
      meta_data.GetObservationTimesSet(sequence.sequenceId);
  result->SetObservationTimes(
      std::vector<double>(observation_times.begin(), observation_times.end()));
  result->SetUVW(uvw);

  return TimeFrequencyMetaDataCPtr{result.release()};
}

static std::unique_ptr<BaselineData> GetData(
    BaselineReader& reader, const MSMetaData::Sequence& sequence,
    const imagesets::ImageSetIndex& index) {
  std::vector<UVW> uvw;
  const TimeFrequencyData data = reader.GetNextResult(uvw);
  const TimeFrequencyMetaDataCPtr meta_data =
      GetMetaData(reader, sequence, uvw);
  return std::make_unique<imagesets::BaselineData>(std::move(data),
                                                   std::move(meta_data), index);
}

std::unique_ptr<BaselineData> MultiBandMsImageSet::CombineData(
    const ImageSetIndex& index) {
  const MSMetaData::Sequence& sequence = GetSequence(index);

  std::vector<std::unique_ptr<BaselineData>> data;
  size_t height = 0;
  for (std::unique_ptr<BaselineReader>& reader : readers_) {
    data.emplace_back(GetData(*reader, sequence, index));
    height += data.back()->Data().ImageHeight();
  }
  return JoinedSPWSet::CombineBaselineData(std::move(data), height, index);
}

void MultiBandMsImageSet::AddReadRequest(const ImageSetIndex& index) {
  const size_t kStartTimeIndex = 0;

  const MSMetaData::Sequence& sequence = sequences_[GetSequenceIndex(index)];
  const size_t end_time_index = EndTimeIndex(sequence.sequenceId);
  for (std::unique_ptr<BaselineReader>& reader : readers_) {
    reader->AddReadRequest(sequence.antenna1, sequence.antenna2, sequence.spw,
                           sequence.sequenceId, kStartTimeIndex,
                           end_time_index);
  }
  read_requests_.emplace_back(index);
}

void MultiBandMsImageSet::PerformReadRequests(ProgressListener& progress) {
  if (!data_.empty()) {
    throw std::runtime_error(
        "PerformReadRequest() called, but a previous read request was not "
        "completely processed.");
  }

  for (size_t i = 0; i != readers_.size(); ++i) {
    SubTaskListener listener(progress, i, readers_.size());
    readers_[i]->PerformReadRequests(listener);
  }
  progress.OnFinish();

  for (const ImageSetIndex& index : read_requests_) {
    data_.emplace_back(CombineData(index));
  }

  read_requests_.clear();
}

std::unique_ptr<BaselineData> MultiBandMsImageSet::GetNextRequested() {
  std::unique_ptr<BaselineData> result = std::move(data_.front());
  data_.pop_front();

  if (result->Data().IsEmpty()) {
    throw std::runtime_error(
        "Calling GetNextRequested(), but requests were not read with "
        "LoadRequests.");
  }
  return result;
}

}  // namespace imagesets

namespace imagesets {
void MultiBandMsImageSet::AddWriteFlagsTask(const ImageSetIndex& index,
                                            std::vector<Mask2DCPtr>& flags) {
  const size_t n_polarizations = readers_[0]->Polarizations().size();
  if (flags.size() > n_polarizations) {
    throw std::runtime_error(
        "Trying to write more polarizations to image set than available");
  }

  std::vector<Mask2DCPtr> all_flags;
  if (flags.size() < n_polarizations) {
    if (flags.size() != 1) {
      throw std::runtime_error(
          "Incorrect number of polarizations in write action");
    }

    all_flags.resize(n_polarizations, flags[0]);
  } else {
    all_flags = flags;
  }

  size_t offset = 0;
  const MSMetaData::Sequence& sequence = GetSequence(index);
  for (size_t i = 0; i < readers_.size(); ++i) {
    std::vector<Mask2DCPtr> reader_flags;
    const size_t count = channels_per_band_[i];
    for (size_t j = 0; j < all_flags.size(); ++j)
      reader_flags.emplace_back(
          Mask2D::CreatePtrFromRows(*all_flags[j], offset, count));

    offset += count;

    readers_[i]->AddWriteTask(reader_flags, sequence.antenna1,
                              sequence.antenna2, 0, sequence.sequenceId);
  }
}

void MultiBandMsImageSet::PerformWriteFlagsTask() {
  for (std::unique_ptr<BaselineReader>& reader : readers_)
    reader->PerformFlagWriteRequests();
}

}  // namespace imagesets
