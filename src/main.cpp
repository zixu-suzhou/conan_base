#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "mbox_time.hpp"

constexpr int loops = 1'000;

int main() {
  std::thread t1([] {
    std::vector<std::chrono::nanoseconds> times;

    for (int i = 0; i < loops; i++) {
      std::chrono::nanoseconds time = mbox_data_time();
      std::cout << time.count() << std::endl;
      times.push_back(time);
    }
  });
  std::thread t2([] {
    std::vector<std::chrono::nanoseconds> times;

    for (int i = 0; i < loops; i++) {
      std::chrono::nanoseconds time = mbox_data_time();
      std::cout << time.count() << std::endl;
      times.push_back(time);
    }
  });
  std::thread t3([] {
    std::vector<std::chrono::nanoseconds> times;

    for (int i = 0; i < loops; i++) {
      std::chrono::nanoseconds time = mbox_UTC();
      std::cout << time.count() << std::endl;
      times.push_back(time);
    }
  });
  std::thread t4([] {
    std::vector<std::chrono::nanoseconds> times;

    for (int i = 0; i < loops; i++) {
      std::chrono::nanoseconds time = mbox_UTC();
      std::cout << time.count() << std::endl;
      times.push_back(time);
    }
  });

  t1.join();
  t2.join();
  t3.join();
  t4.join();

  if (true) {
    std::chrono::nanoseconds t1, t2, t3, t4, t5;
    t1 = mbox_UTC();
    for (int i = 0; i < loops; i++) {
      t2 = mbox_UTC();
    }
    t3 = mbox_UTC();
    for (int i = 0; i < loops; i++) {
      t4 = mbox_data_time();
    }
    t5 = mbox_UTC();

    std::cout << t1.count() << std::endl
              << t2.count() << std::endl
              << t3.count() << std::endl
              << t4.count() << std::endl
              << t5.count() << std::endl;

    std::cout << (t3 - t1).count() << std::endl
              << (t5 - t3).count() << std::endl;
  }

  for (;;) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto t1 = std::chrono::nanoseconds(
        std::chrono::steady_clock::now().time_since_epoch());
    auto data_time = mbox_data_time();
    auto t2 = std::chrono::nanoseconds(
        std::chrono::steady_clock::now().time_since_epoch());
    auto system_time = mbox_data_time();
    auto t3 = std::chrono::nanoseconds(
        std::chrono::steady_clock::now().time_since_epoch());

    std::cout << "data_time: " << data_time.count() << "ns in " << (t2 - t1).count() << "ns\n";
    std::cout << "system_time: " << system_time.count() << "ns in " << (t3 - t2).count() << "ns\n";
  }
  return 0;
}
