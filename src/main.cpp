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
#include "status_monitor.h"
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <vector>

using namespace std;
using namespace CameraService;

char *randstr(char *str, const int len) {
  srand(time(NULL));
  int i;
  for (i = 0; i < len; ++i) {
    switch ((rand() % 3)) {
    case 1:
      str[i] = 'A' + rand() % 26;
      break;
    case 2:
      str[i] = 'a' + rand() % 26;
      break;
    default:
      str[i] = '0' + rand() % 10;
      break;
    }
  }
  str[++i] = '\0';
  return str;
}

void std_reporter(
    const std::vector<StatusMonitorAbstract::StatusMonitorReport> &reports) {
  std::vector<StatusMonitorAbstract::StatusMonitorReport>::const_iterator iter =
      reports.begin();
  while (iter != reports.end()) {
    char precision_s[20] = {0};
    if (iter->report_type ==
        StatusMonitorAbstract::StatusMonitorReport::HEART_BEAT) {
      snprintf(precision_s, 19, "%.2f", iter->fps);
      printf("report pipeline %s, fps = %s, frame_loss = %s, sync = %s, delay "
             "= %lu, online = %s\n",
             iter->pipeline_name.c_str(), precision_s, iter->frame_loss.c_str(),
             iter->sync == true ? "true" : "false", iter->delay_us,
             iter->online == true ? "true" : "false");
    } else if (iter->report_type ==
               StatusMonitorAbstract::StatusMonitorReport::WARNING) {
      printf("report pipeline %s warning[%d]\n", iter->pipeline_name.c_str(),
             iter->warning);
    }
    iter++;
  }
  return;
};

int main(int argc, char *argv[]) {
  StatusMonitor::getInstance();
  StatusMonitor::getInstance().run_forever();

  StatusMonitorAbstract::PipelineInformation meta;
  meta.fps = 10;
  meta.pipeline_name = "front_far";
  StatusMonitor::getInstance().pipeline_registration(meta);
  meta.pipeline_name = "front_wide";
  StatusMonitor::getInstance().pipeline_registration(meta);
  meta.pipeline_name = "front_fisheye";
  StatusMonitor::getInstance().pipeline_registration(meta);
  StatusMonitor::getInstance().set_reporter_callback(&std_reporter);
  sleep(2);

  StatusMonitorAbstract::StatusMonitorFrame signal;
  uint32_t count = 0;
  while (count <= 1000000) {
    usleep(90000 + rand() % 10000);
    auto micros =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
    signal.pipeline_name = "front_wide";
    signal.sensor_timestamp_us = (uint64_t)micros;
    signal.receive_timestamp_us = (uint64_t)micros + (rand() % 800);
    StatusMonitor::getInstance().signal(signal);
    if (0 == count % 2) {
      signal.pipeline_name = "front_far";
      StatusMonitor::getInstance().signal(signal);
    }
    if (0 == count % 3) {
      signal.pipeline_name = "front_fisheye";
      StatusMonitor::getInstance().signal(signal);
    }
    count++;
  }

  StatusMonitor::getInstance().stop();

  return 0;
}
