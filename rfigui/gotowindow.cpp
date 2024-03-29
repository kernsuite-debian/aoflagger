#include "gotowindow.h"

#include <sstream>

#include <gtkmm/treemodel.h>

#include "../imagesets/msimageset.h"
#include "../imagesets/multibandmsimageset.h"

#include "controllers/rfiguicontroller.h"

#include "rfiguiwindow.h"

static const std::vector<MSMetaData::Sequence>& GetSequences(
    const imagesets::IndexableSet* image_set) {
  if (const auto* multi_band_image_set =
          dynamic_cast<const imagesets::MultiBandMsImageSet*>(image_set))
    return multi_band_image_set->Sequences();

  return image_set->Reader()->MetaData().GetSequences();
}

GoToWindow::GoToWindow(RFIGuiWindow& rfiGuiWindow)
    : Gtk::Window(),
      _antenna1Frame("Antenna 1"),
      _antenna2Frame("Antenna 2"),
      _bandFrame("Band"),
      _sequenceFrame("Time sequence"),
      _loadButton("Load"),
      _keepOpenCB("Keep window open"),
      _rfiGuiWindow(rfiGuiWindow),
      _imageSet(&static_cast<imagesets::IndexableSet&>(
          rfiGuiWindow.Controller().GetImageSet())) {
  set_default_size(0, 500);
  _antennaeStore = Gtk::ListStore::create(_antennaModelColumns);
  _bandStore = Gtk::ListStore::create(_bandModelColumns);
  _sequenceStore = Gtk::ListStore::create(_sequenceModelColumns);

  const std::vector<MSMetaData::Sequence>& _sequences = GetSequences(_imageSet);

  const imagesets::ImageSetIndex& setIndex =
      _rfiGuiWindow.Controller().GetImageSetIndex();
  const unsigned antenna1Index = _imageSet->GetAntenna1(setIndex);
  const unsigned antenna2Index = _imageSet->GetAntenna2(setIndex);
  const unsigned bandIndex = _imageSet->GetBand(setIndex);
  const unsigned sequenceIndex = _imageSet->GetSequenceId(setIndex);

  // First, the sequences are iterated to get all antenna indices.
  std::set<size_t> set;
  for (const MSMetaData::Sequence& seq : _sequences) {
    set.insert(seq.antenna1);
    set.insert(seq.antenna2);
  }

  Gtk::TreeModel::iterator a1Row, a2Row, bandRow, sequenceRow;

  // Now we make a store that contains all antennas. This store is shared for
  // both a1 and a2 views.
  for (std::set<size_t>::const_iterator i = set.begin(); i != set.end(); ++i) {
    const Gtk::TreeModel::iterator iter = _antennaeStore->append();
    (*iter)[_antennaModelColumns.antennaIndex] = *i;
    const AntennaInfo antenna = _imageSet->GetAntennaInfo(*i);
    (*iter)[_antennaModelColumns.antennaName] = antenna.name;
    if (antenna1Index == *i) a1Row = iter;
    if (antenna2Index == *i) a2Row = iter;
  }

  const size_t bandCount = _imageSet->BandCount();
  for (size_t i = 0; i < bandCount; ++i) {
    const Gtk::TreeModel::iterator iter = _bandStore->append();
    (*iter)[_bandModelColumns.bandIndex] = i;
    std::stringstream desc;
    BandInfo band = _imageSet->GetBandInfo(i);
    desc << Frequency::ToString(band.channels.front().frequencyHz);
    desc << " - ";
    desc << Frequency::ToString(band.channels.back().frequencyHz);
    (*iter)[_bandModelColumns.bandDescription] = desc.str();
    if (i == bandIndex) bandRow = iter;
  }

  const size_t sequenceIdCount = _imageSet->SequenceCount();
  size_t lastSeqIndex = 0;
  for (size_t i = 0; i < sequenceIdCount; ++i) {
    const Gtk::TreeModel::iterator iter = _sequenceStore->append();
    (*iter)[_sequenceModelColumns.sequenceIndex] = i;
    std::stringstream desc;
    // Find some index that has this sequence.
    while (_sequences[lastSeqIndex].sequenceId != i) {
      ++lastSeqIndex;
    }
    std::optional<imagesets::ImageSetIndex> index(_imageSet->Index(
        _sequences[lastSeqIndex].antenna1, _sequences[lastSeqIndex].antenna2,
        _sequences[lastSeqIndex].spw, i));
    const size_t fIndex = _imageSet->GetField(*index);
    const FieldInfo field = _imageSet->GetFieldInfo(fIndex);
    desc << field.name << " (" << fIndex << ')';
    (*iter)[_sequenceModelColumns.sequenceDescription] = desc.str();
    if (i == sequenceIndex) sequenceRow = iter;
  }

  _antenna1View.set_model(_antennaeStore);
  _antenna1View.append_column("Index", _antennaModelColumns.antennaIndex);
  _antenna1View.append_column("Name", _antennaModelColumns.antennaName);
  _antenna1View.set_size_request(150, 512);
  _antenna1View.get_selection()->select(a1Row);
  _antenna1View.get_selection()->signal_changed().connect([&] { onChange(); });
  _antenna1Scroll.add(_antenna1View);
  _antenna1Frame.add(_antenna1Scroll);
  _antenna1Scroll.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  _hBox.pack_start(_antenna1Frame);

  _antenna2View.set_model(_antennaeStore);
  _antenna2View.append_column("Index", _antennaModelColumns.antennaIndex);
  _antenna2View.append_column("Name", _antennaModelColumns.antennaName);
  _antenna2View.set_size_request(150, 512);
  _antenna2View.get_selection()->select(a2Row);
  _antenna2View.get_selection()->signal_changed().connect([&] { onChange(); });
  _antenna2Scroll.add(_antenna2View);
  _antenna2Frame.add(_antenna2Scroll);
  _antenna2Scroll.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  _hBox.pack_start(_antenna2Frame);

  _bandView.set_model(_bandStore);
  _bandView.append_column("Index", _bandModelColumns.bandIndex);
  _bandView.append_column("Description", _bandModelColumns.bandDescription);
  //_bandView.set_size_request(-1, 512);
  _bandView.get_selection()->select(bandRow);
  _bandView.get_selection()->signal_changed().connect([&] { onChange(); });
  _bandScroll.add(_bandView);
  _bandFrame.add(_bandScroll);
  _bandScroll.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  _bandFrameBox.pack_start(_bandFrame);

  _sequenceView.set_model(_sequenceStore);
  _sequenceView.append_column("Index", _sequenceModelColumns.sequenceIndex);
  _sequenceView.append_column("Source",
                              _sequenceModelColumns.sequenceDescription);
  //_sequenceView.set_size_request(-1, 512);
  _sequenceView.get_selection()->select(sequenceRow);
  _sequenceView.get_selection()->signal_changed().connect([&] { onChange(); });
  _sequenceScroll.add(_sequenceView);
  _sequenceFrame.add(_sequenceScroll);
  _sequenceScroll.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  _bandFrameBox.pack_start(_sequenceFrame);

  _hBox.pack_start(_bandFrameBox);

  _vBox.pack_start(_hBox);

  _hBottomBox.pack_start(_keepOpenCB, false, false);
  _hBottomBox.pack_start(_label);

  _loadButton.signal_clicked().connect(
      sigc::mem_fun(*this, &GoToWindow::onLoadClicked));
  _buttonBox.pack_start(_loadButton);
  _hBottomBox.pack_end(_buttonBox);

  _vBox.pack_start(_hBottomBox, Gtk::PACK_SHRINK, 0);

  add(_vBox);
  _vBox.show_all();

  onChange();
}

GoToWindow::~GoToWindow() {}

std::optional<imagesets::ImageSetIndex> GoToWindow::getIndex() {
  const Glib::RefPtr<Gtk::TreeSelection> a1 = _antenna1View.get_selection();
  const Gtk::TreeModel::iterator iterA1 = a1->get_selected();

  const Glib::RefPtr<Gtk::TreeSelection> a2 = _antenna2View.get_selection();
  const Gtk::TreeModel::iterator iterA2 = a2->get_selected();

  const Glib::RefPtr<Gtk::TreeSelection> b = _bandView.get_selection();
  const Gtk::TreeModel::iterator iterB = b->get_selected();

  const Glib::RefPtr<Gtk::TreeSelection> s = _sequenceView.get_selection();
  const Gtk::TreeModel::iterator iterS = s->get_selected();

  if (iterA1 && iterA2 && iterB && iterS) {
    const Gtk::TreeModel::Row a1Row = *iterA1;
    const Gtk::TreeModel::Row a2Row = *iterA2;
    const Gtk::TreeModel::Row bRow = *iterB;
    const Gtk::TreeModel::Row sRow = *iterS;
    const size_t a1Index = a1Row[_antennaModelColumns.antennaIndex];
    const size_t a2Index = a2Row[_antennaModelColumns.antennaIndex];
    const size_t bIndex = bRow[_bandModelColumns.bandIndex];
    const size_t sIndex = sRow[_sequenceModelColumns.sequenceIndex];
    return _imageSet->Index(a1Index, a2Index, bIndex, sIndex);
  }
  return std::optional<imagesets::ImageSetIndex>();
}

void GoToWindow::onChange() {
  std::optional<imagesets::ImageSetIndex> index = getIndex();
  if (index) {
    const size_t a1Index = _imageSet->GetAntenna1(*index),
                 a2Index = _imageSet->GetAntenna2(*index);
    const AntennaInfo a1 = _imageSet->GetAntennaInfo(a1Index),
                      a2 = _imageSet->GetAntennaInfo(a2Index);
    const double distance = a1.position.Distance(a2.position);
    std::ostringstream str;
    str << a1.name + " x " + a2.name + " (";
    if (distance < 1000)
      str << std::round(distance) << " m";
    else
      str << std::round(distance / 100.0) / 10.0 << " km";
    str << ")";
    _label.set_text(str.str());
    _loadButton.set_sensitive(true);
  } else {
    _loadButton.set_sensitive(false);
  }
}

void GoToWindow::onLoadClicked() {
  std::optional<imagesets::ImageSetIndex> index = getIndex();
  if (index) {
    _rfiGuiWindow.SetImageSetIndex(*index);
    if (!_keepOpenCB.get_active()) hide();
  }
}
