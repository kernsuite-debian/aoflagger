#ifndef LUA_DATA_H
#define LUA_DATA_H

#include "../structures/timefrequencydata.h"
#include "../structures/timefrequencymetadata.h"

#include <algorithm>

namespace aoflagger_lua {
class Data {
 public:
  struct Context {
    std::vector<Data*> list;
    Context() = default;

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    Context(Context&&) = default;
    Context& operator=(Context&&) = default;
  };

  Data(Context& context) : _context(&context), _persistent(false) {
    context.list.emplace_back(this);
  }

  Data(const Data& source)
      : _tfData(source._tfData),
        _metaData(source._metaData),
        _context(source._context),
        _persistent(source._persistent) {
    _context->list.emplace_back(this);
  }

  Data(Data&& source)
      : _tfData(std::move(source._tfData)),
        _metaData(std::move(source._metaData)),
        _context(source._context),
        _persistent(source._persistent) {
    _context->list.emplace_back(this);
  }

  Data(const TimeFrequencyData& tfData, TimeFrequencyMetaDataCPtr metaData,
       Context& context)
      : _tfData(tfData),
        _metaData(metaData),
        _context(&context),
        _persistent(false) {
    context.list.emplace_back(this);
  }

  Data(TimeFrequencyData&& tfData, TimeFrequencyMetaDataCPtr metaData,
       Context& context) noexcept
      : _tfData(std::move(tfData)),
        _metaData(metaData),
        _context(&context),
        _persistent(false) {
    context.list.emplace_back(this);
  }

  ~Data() noexcept {
    // If this object is persistent, it might exist after the context was
    // deleted In that case, we don't update context.
    if (!_persistent) {
      auto iter = std::find(_context->list.begin(), _context->list.end(), this);
      // The list might be cleared already (at the end of the strategy), so make
      // sure the data was found.
      if (iter != _context->list.end()) _context->list.erase(iter);
    }
  }

  Data& operator=(Data&& rhs) {
    _tfData = std::move(rhs._tfData);
    _metaData = std::move(rhs._metaData);
    _context = rhs._context;
    _persistent = rhs._persistent;
    return *this;
  }

  Data operator-(const Data& other) const {
    return Data(TimeFrequencyData::MakeFromDiff(_tfData, other.TFData()),
                _metaData, *_context);
  }

  void clear() {
    _tfData = TimeFrequencyData();
    _metaData.reset();
  }

  void clear_mask() noexcept { _tfData.SetNoMask(); }

  Data convert_to_polarization(aocommon::PolarizationEnum polarization) const {
    return Data(_tfData.Make(polarization), _metaData, *_context);
  }

  Data convert_to_complex(enum TimeFrequencyData::ComplexRepresentation
                              complexRepresentation) const {
    return Data(_tfData.Make(complexRepresentation), _metaData, *_context);
  }

  Data copy() const { return Data(_tfData, _metaData, *_context); }

  void join_mask(const Data& other) { _tfData.JoinMask(other._tfData); }

  Data make_complex() const {
    return Data(_tfData.MakeFromComplexCombination(_tfData, _tfData), _metaData,
                *_context);
  }

  void set_visibilities(const Data& image_data) {
    const TimeFrequencyData& source = image_data._tfData;
    const size_t imageCount = source.ImageCount();
    if (imageCount != _tfData.ImageCount()) {
      std::ostringstream s;
      s << "set_image() was executed with incompatible polarizations: input "
           "had "
        << imageCount << ", destination had " << _tfData.ImageCount();
      throw std::runtime_error(s.str());
    }
    for (size_t i = 0; i != imageCount; ++i)
      _tfData.SetImage(i, source.GetImage(i));
  }

  void set_polarization_data(aocommon::PolarizationEnum polarization,
                             const Data& data) {
    size_t polIndex = _tfData.GetPolarizationIndex(polarization);
    _tfData.SetPolarizationData(polIndex, data._tfData);
  }

  TimeFrequencyData& TFData() noexcept { return _tfData; }
  const TimeFrequencyData& TFData() const noexcept { return _tfData; }

  const TimeFrequencyMetaDataCPtr& MetaData() const noexcept {
    return _metaData;
  }

  Context& GetContext() const { return *_context; }

  bool is_persistent() const { return _persistent; }
  void set_persistent(bool persistent) { _persistent = persistent; }

 private:
  TimeFrequencyData _tfData;
  TimeFrequencyMetaDataCPtr _metaData;
  Context* _context;
  bool _persistent;
};
}  // namespace aoflagger_lua

#endif
