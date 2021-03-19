/* event_browser_ctrl.cc */

// http://drdobbs.com/cpp/184401518

#include <EventBrowser/view/event_browser.h>
#include <EventBrowser/view/event_browser_ctrl.h>
#include <EventBrowser/view/options_manager.h>

#include <iostream>

#include <boost/bind.hpp>

namespace snemo {

namespace visualization {

namespace view {

void event_browser_ctrl::set_event_browser(event_browser& browser_) { browser = &browser_; }

event_browser_ctrl::event_browser_ctrl(event_browser& browser_, size_t max_counts_) {
  // test
  browser = nullptr;
  browser_thread = nullptr;
  event_mutex = nullptr;
  event_available_condition = nullptr;

  event_availability_status = event_browser_ctrl::NOT_AVAILABLE_FOR_ROOT;
  stop_requested = false;
  counts = 0;
  max_counts = max_counts_;

  set_event_browser(browser_);

  event_mutex = new boost::mutex;
  event_available_condition = new boost::condition;
}

event_browser_ctrl::~event_browser_ctrl() {
  set_stop_requested();
  if (event_mutex != nullptr) {
    boost::mutex::scoped_lock lock(*event_mutex);
    event_availability_status = event_browser_ctrl::ABORT;
    event_available_condition->notify_one();
  }
  if (browser_thread != nullptr) {
    browser_thread->join();
  }

  if (event_available_condition != nullptr) {
    delete event_available_condition;
    event_available_condition = nullptr;
  }
  if (event_mutex != nullptr) {
    delete event_mutex;
    event_mutex = nullptr;
  }

  if (browser_thread != nullptr) {
    delete browser_thread;
    browser_thread = nullptr;
  }
  browser = nullptr;
}

void event_browser_ctrl::start() {
  browser_thread = new boost::thread(boost::bind(&event_browser_ctrl::start_browsing_event, this));
}

void event_browser_ctrl::start_browsing_event() {
  {
    boost::mutex::scoped_lock lock(*event_mutex);
    while (event_availability_status == event_browser_ctrl::NOT_AVAILABLE_FOR_ROOT) {
      event_available_condition->wait(*event_mutex);
    }
  }
  browser->start_threading();
}

void event_browser_ctrl::set_stop_requested() { stop_requested = true; }

bool event_browser_ctrl::is_stop_requested() const { return stop_requested; }

}  // end of namespace view

}  // end of namespace visualization

}  // end of namespace snemo

// end of event_browser_ctrl.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
