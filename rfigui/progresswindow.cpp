#include "progresswindow.h"

#include <iostream>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <gtkmm/messagedialog.h>

ProgressWindow::ProgressWindow()
    : _blockProgressSignal(false),
      _currentTaskTitleLabel("Current task:"),
      _currentTaskLabel("-"),
      _timeElapsedTitleLabel("Time elapsed:"),
      _timeElapsedLabel("-"),
      _timeEstimatedTitleLabel("Estimated time:"),
      _timeEstimatedLabel("-"),
      _started(false),
      _exceptionQueued(false),
      _progress(0),
      _maxProgress(0),
      _finished(false) {
  set_default_size(350, 60);

  _currentTaskTitleLabel.set_vexpand(true);
  _currentTaskTitleLabel.set_alignment(Gtk::ALIGN_END);
  _currentTaskTitleLabel.set_padding(10, 2);
  _grid.attach(_currentTaskTitleLabel, 0, 0, 1, 1);
  _currentTaskLabel.set_alignment(Gtk::ALIGN_START);
  _grid.attach(_currentTaskLabel, 1, 0, 1, 1);
  _timeElapsedTitleLabel.set_vexpand(true);
  _timeElapsedTitleLabel.set_alignment(Gtk::ALIGN_END);
  _timeElapsedTitleLabel.set_padding(10, 2);
  _grid.attach(_timeElapsedTitleLabel, 0, 1, 1, 1);
  _timeElapsedLabel.set_alignment(Gtk::ALIGN_START);
  _grid.attach(_timeElapsedLabel, 1, 1, 1, 1);
  _timeEstimatedTitleLabel.set_vexpand(true);
  _timeEstimatedTitleLabel.set_alignment(Gtk::ALIGN_END);
  _timeEstimatedTitleLabel.set_padding(10, 2);
  _grid.attach(_timeEstimatedTitleLabel, 0, 2, 1, 1);
  _timeEstimatedLabel.set_alignment(Gtk::ALIGN_START);
  _grid.attach(_timeEstimatedLabel, 1, 2, 1, 1);
  _progressBar.set_hexpand(true);
  _progressBar.set_vexpand(true);
  _grid.attach(_progressBar, 0, 3, 2, 1);

  add(_grid);
  _grid.show_all();

  _progressChangeSignal.connect(
      sigc::mem_fun(*this, &ProgressWindow::updateProgress));
}

ProgressWindow::~ProgressWindow() {}

void ProgressWindow::updateProgress() {
  if (!_blockProgressSignal) {
    if (!_started) {
      _startTime = boost::posix_time::microsec_clock::local_time();
      _lastUpdate = _startTime - boost::posix_time::time_duration(0, 0, 1);
      _started = true;
    }

    std::unique_lock<std::mutex> lock(_mutex);

    boost::posix_time::ptime now =
        boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration sinceLast = now - _lastUpdate;
    boost::posix_time::time_duration updTime = boost::posix_time::millisec(100);
    bool doUpdate = sinceLast >= updTime;
    if (doUpdate) {
      _lastUpdate = now;
      boost::posix_time::time_duration duration = now - _startTime;
      std::stringstream timeStr;
      timeStr << duration;
      _timeElapsedLabel.set_text(timeStr.str());

      std::string taskDesc = _taskDescription;
      double progress = std::min(
          1.0, std::max(0.0, double(_progress) / double(_maxProgress)));

      _currentTaskLabel.set_text(taskDesc);
      _progressBar.set_fraction(progress);

      if (progress > 0.0) {
        std::stringstream estimatedTimeStr;
        boost::posix_time::time_duration estimated =
            (boost::posix_time::microsec_clock::local_time() - _startTime) *
            (int)(1000000.0 * (1.0 - progress)) / (int)(1000000.0 * progress);
        estimated = boost::posix_time::seconds(estimated.total_seconds());
        estimatedTimeStr << estimated;
        _timeEstimatedLabel.set_text(estimatedTimeStr.str());
      }
    }

    bool hasException = _exceptionQueued;
    if (hasException) {
      _exceptionQueued = false;
      _blockProgressSignal = true;
      if (_signalError.empty()) {
        // Default handler: show a message dialog
        std::string errMsg = std::string("An exception was thrown of type '") +
                             _exceptionType + ("' -- ") + _exceptionDescription;
        lock.unlock();
        Gtk::MessageDialog dialog(*this, errMsg, false, Gtk::MESSAGE_ERROR);
        dialog.run();
      } else {
        std::string errDescr =
            _exceptionDescription;  // local copy to unlock mutex
        lock.unlock();
        _signalError(errDescr);
      }
      _signalFinished(false);
      lock.lock();
      _blockProgressSignal = false;
    } else if (_finished) {
      _finished = false;  // Reset in case window is reused
      lock.unlock();

      _currentTaskLabel.set_text("-");
      _timeEstimatedLabel.set_text("-");
      _progressBar.set_fraction(1.0);

      // Parent might delete this window in this call -- don't do anything
      // after.
      _signalFinished(true);
    }
  }
}

void ProgressWindow::OnStartTask(const std::string &description) {
  std::unique_lock<std::mutex> lock(_mutex);
  _taskDescription = description;
  lock.unlock();

  _progressChangeSignal();
}

void ProgressWindow::OnFinish() {
  std::unique_lock<std::mutex> lock(_mutex);
  _finished = true;
  lock.unlock();

  _progressChangeSignal();
}

void ProgressWindow::OnProgress(size_t progress, size_t maxProgress) {
  std::unique_lock<std::mutex> lock(_mutex);
  _progress = progress;
  _maxProgress = maxProgress;
  lock.unlock();

  _progressChangeSignal();
}

void ProgressWindow::OnException(std::exception &thrownException) {
  std::unique_lock<std::mutex> lock(_mutex);
  _exceptionQueued = true;
  _exceptionDescription = thrownException.what();
  _exceptionType = typeid(thrownException).name();
  lock.unlock();

  _progressChangeSignal();
}
