/*****************************************************************************
 * Copyright (C) 2022 Momenta Technology Co., Ltd. All rights reserved.
 *
 * No.58 Qinglonggang Rd, Suzhou, Jiangsu, PR China, contact@momenta.ai
 *
 * https://www.momenta.cn/
 *
 * Filename         : main.cpp
 * Created          : 2022-09-03 12:23
 * Last modified    : 2022-09-03 12:24
 * Author           : denny.zhang <denny.zhang@momenta.ai>
 * Description      :
 *****************************************************************************/
#include "time_provider.h"
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
  uint64_t s = 0, ns = 0;

  auto start = std::chrono::system_clock::now();
  TIME_RESULT ret = GetTime(s, ns);
  auto end = std::chrono::system_clock::now();
  auto duration = end - start;
  double time_cost = static_cast<double>(duration.count())/1000.0;
  if (ret != TIME_RESULT::K_RET_OK) {
    (void)fprintf(stderr, "[Sample] Get time error, error code is %d.\n",
                  static_cast<int>(ret));
  }

  uint64_t during{s * 1000000000 + ns};

  std::cout << "Sample: get GTS: " << during << ", s: " << s << " ns: " << ns
            << " API exec cost: " << time_cost << "us" << std::endl;


  return 0;
}
