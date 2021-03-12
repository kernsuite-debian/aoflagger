#ifndef PROGRESSWINDOW_H
#define PROGRESSWINDOW_H

#include "../util/progresslistener.h"

#include <glibmm.h>

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/window.h>

#include <mutex>
#include <sstream>

class ProgressWindow : public Gtk::Window, public ProgressListener {
 public:
  explicit ProgressWindow();
  ~ProgressWindow();

  virtual void OnStartTask(const std::string& description) final override;
  virtual void OnProgress(size_t progress, size_t maxProgress) final override;
  virtual void OnFinish() final override;
  virtual void OnException(std::exception& thrownException) final override;

  sigc::signal<void, bool /*errors occurred? */>& SignalFinished() {
    return _signalFinished;
  }
  sigc::signal<void, const std::string&>& SignalError() { return _signalError; }

 private:
  void updateProgress();
  Glib::Dispatcher _progressChangeSignal;
  bool _blockProgressSignal;
  std::mutex _mutex;
  sigc::signal<void, bool> _signalFinished;
  sigc::signal<void, const std::string&> _signalError;

  Gtk::Label _currentTaskTitleLabel, _currentTaskLabel, _timeElapsedTitleLabel,
      _timeElapsedLabel, _timeEstimatedTitleLabel, _timeEstimatedLabel;

  Gtk::Grid _grid;
  Gtk::ProgressBar _progressBar;

  double _progressFraction;
  boost::posix_time::ptime _startTime, _lastUpdate;
  bool _started;
  bool _exceptionQueued;
  std::string _exceptionType;
  std::string _exceptionDescription;

  std::string _taskDescription;
  size_t _progress, _maxProgress;
  bool _finished;
};

#endif
