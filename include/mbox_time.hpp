#pragma once

#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

#ifndef CLOCK_INVALID
#define CLOCK_INVALID -1
#endif

#define CLOCKFD 3
#define FD_TO_CLOCKID(fd) ((clockid_t)((((unsigned int)~fd) << 3) | CLOCKFD))

// assume /dev/data_phc symbolic link to real_phc device(particular /dev/ptpX)
constexpr const char *mbox_time_data_phc() { return "/dev/data_phc"; }
constexpr std::chrono::nanoseconds mbox_data_time_sync_period() {
  return std::chrono::seconds(2);
}

// constexpr char mbox_time_data_phc[] = "/dev/data_phc";
// constexpr std::chrono::nanoseconds mbox_data_time_sync_period(2'000'000'000);

class MBoxTime {
 public:
  MBoxTime();
  ~MBoxTime();

  std::chrono::nanoseconds get_data_time();
  std::chrono::nanoseconds get_UTC();

 private:
  void init();
  bool phc_open();
  bool sync();

 private:
  const char *data_phc_ = mbox_time_data_phc();
  const std::chrono::nanoseconds sync_period_ = mbox_data_time_sync_period();

  std::mutex clock_m_;
  int fd_ = -1;
  clockid_t clkid_ = CLOCK_INVALID;

  std::mutex offset_m_;
  std::chrono::nanoseconds offset_{0};
  std::chrono::nanoseconds last_sync_time_{0};
};

inline MBoxTime &mbox_time() {
#if __cplusplus < 201103L
#error \
    "c++11 is required for thread-safe static local variables initialization, \
     https://en.cppreference.com/w/cpp/language/storage_duration#Static_local_variables"
#endif

  static MBoxTime mbox_time;
  return mbox_time;
}

inline std::chrono::nanoseconds mbox_data_time() {
  return mbox_time().get_data_time();
}

inline std::chrono::nanoseconds mbox_UTC() { return mbox_time().get_UTC(); }

// clockid_t mbox_time__a_dummy_variable_for_eager_initialization =
// mbox_time().get_clock_id();

MBoxTime::MBoxTime() { init(); }

MBoxTime::~MBoxTime() { close(fd_); }

std::chrono::nanoseconds MBoxTime::get_data_time() {
  auto steady_time = std::chrono::nanoseconds(
      std::chrono::steady_clock::now().time_since_epoch());

  std::lock_guard<std::mutex> lock(offset_m_);
  if (steady_time - last_sync_time_ > sync_period_) {
    // readlink check may needed here
    if (!sync()) {
      std::lock_guard<std::mutex> lock(clock_m_);
      init();
    }
  }

  return steady_time + offset_;
}

std::chrono::nanoseconds MBoxTime::get_UTC() {
  return std::chrono::nanoseconds(
      std::chrono::system_clock::now().time_since_epoch());
}

void MBoxTime::init() {
  if (phc_open()) {
    if (sync()) {
      return;
    }
    close(fd_);
  }

  fd_ = -1;
  clkid_ = CLOCK_INVALID;
}

bool MBoxTime::phc_open() {
  clockid_t clkid;
  int fd;

  fd = open(data_phc_, O_RDONLY);
  if (fd < 0) {
    return false;
  }

  clkid = FD_TO_CLOCKID(fd);

  fd_ = fd;
  clkid_ = clkid;

  return true;
}

bool MBoxTime::sync() {
  struct timespec ts;

  memset(&ts, 0, sizeof(ts));
  {
    // auto start = std::chrono::nanoseconds(
        // std::chrono::steady_clock::now().time_since_epoch());
    if (clock_gettime(clkid_, &ts) != 0) {
      return false;
    }
    // auto end = std::chrono::nanoseconds(
        // std::chrono::steady_clock::now().time_since_epoch());
    // std::cout << "clock_gettime cost: "  << (end - start).count() << "ns\n";
  }

  auto steady_time = std::chrono::nanoseconds(
      std::chrono::steady_clock::now().time_since_epoch());
  auto data_time = std::chrono::nanoseconds(
      std::chrono::seconds(ts.tv_sec) + std::chrono::nanoseconds(ts.tv_nsec));
  offset_ = data_time - steady_time;
  last_sync_time_ = steady_time;

  return true;
}
