/*****************************************************************************
 * Copyright (C) 2022 Momenta Technology Co., Ltd. All rights reserved.
 *
 * No.58 Qinglonggang Rd, Suzhou, Jiangsu, PR China, contact@momenta.ai
 *
 * https://www.momenta.cn/
 *
 * Filename         : status_monitor.cpp
 * Created          : 2022-09-03 12:24
 * Last modified    : 2022-09-03 12:24
 * Author           : denny.zhang <denny.zhang@momenta.ai>
 * Description      : common frame pipeline status monitor
 *****************************************************************************/
#include "status_monitor.h"
#include <chrono>
#include <cmath>
#include <cstring>
#include <ctime>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <unordered_set>

using namespace std;
namespace CameraService {
static const std::map<std::string,
                      StatusMonitorAbstract::STATUS_MONITOR_WARNING>
    CAMERA_SERVICE_MONITOR_STATUS_MAP = {
        {"ok", StatusMonitorAbstract::SM_STATUS_OK},
        {"lock_lost", StatusMonitorAbstract::SM_LOCK_LOST},
        {"encode_error", StatusMonitorAbstract::SM_ENCODE_ERROR},
        {"hal_lost", StatusMonitorAbstract::SM_DRIVER_ERROR},
        {"read_timeout", StatusMonitorAbstract::SM_FRAME_LOSS},
        {"delay", StatusMonitorAbstract::SM_STATUS_DELAY},
        {"black", StatusMonitorAbstract::SM_DRIVER_ERROR},
        {"change", StatusMonitorAbstract::SM_HARDWARE_CHANGED},
        {"calib_missing", StatusMonitorAbstract::SM_CALIB_LOST},
        {"init_error", StatusMonitorAbstract::SM_INIT_FAIL},
        {"sedres_lock", StatusMonitorAbstract::SM_SEDERS_LOCK}};

struct PipelineHandler {
  StatusMonitor::PipelineInformation meta;
  uint64_t pipeline_start_time_us = 0;
  uint64_t last_report_time = 0;
  uint64_t latest_frame_timestamp_us = 0;
  uint64_t seq = 0;
  uint64_t frame_count_start_time_us = 0;
  uint32_t frame_count = 0;
  uint32_t actual_fps = 0;
  bool sync = false;
  bool online = false;
  uint64_t delay_us = 0;
  std::unordered_set<StatusMonitorAbstract::StatusMonitorWarning,
                     StatusMonitorAbstract::StatusMonitorWarningHashFunc>
      camera_status_set;
};

struct StatusMonitor::StatusMonitorMainHandler {
  std::shared_ptr<std::thread> running_thread;
  bool is_quit;
  uint64_t last_report_time;
  std::mutex pipelines_lock_;
  std::map<std::string, PipelineHandler> pipelines;
  std::mutex signal_queue_lock_;
  std::queue<StatusMonitorFrame> frame_queue;
  std::mutex warning_queue_lock_;
  std::queue<StatusMonitorWarning> warning_queue;
};

StatusMonitor::StatusMonitor() {
  main_handler_ = new StatusMonitorMainHandler();
  main_handler_->is_quit = false;
  main_handler_->running_thread = nullptr;
};
StatusMonitor::StatusMonitor(const StatusMonitor &){};

StatusMonitor::~StatusMonitor() {
  if (nullptr != main_handler_) {
    delete (main_handler_);
  }
};

void StatusMonitor::run_once() {
  StatusMonitorFrame signal;
  StatusMonitorWarning signal_warning;
  auto micros_now =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();

  /* process frame queue */
  while (!main_handler_->frame_queue.empty()) {
    /* pop signal */
    {
      std::lock_guard<std::mutex> lg(main_handler_->signal_queue_lock_);
      signal = main_handler_->frame_queue.front();
      main_handler_->frame_queue.pop();
    }
    // process signal && set start time
    {
      std::lock_guard<std::mutex> lg(main_handler_->pipelines_lock_);
      if (main_handler_->pipelines.find(signal.pipeline_name) !=
          main_handler_->pipelines.end()) {
        if (main_handler_->pipelines[signal.pipeline_name]
                .pipeline_start_time_us == 0) {
          main_handler_->pipelines[signal.pipeline_name]
              .pipeline_start_time_us =
              main_handler_->pipelines[signal.pipeline_name]
                  .frame_count_start_time_us = micros_now;
          main_handler_->pipelines[signal.pipeline_name].last_report_time =
              micros_now; // set system time as last report time
        }
        // calculate frame sync
        int64_t timestamp_diff =
            micros_now - main_handler_->pipelines[signal.pipeline_name]
                             .latest_frame_timestamp_us;
        int64_t frame_interval =
            1000000 / main_handler_->pipelines[signal.pipeline_name].meta.fps;
        if (abs(timestamp_diff - frame_interval) > 1000) {
          main_handler_->pipelines[signal.pipeline_name].sync = false;
        } else {
          main_handler_->pipelines[signal.pipeline_name].sync = true;
        }
        main_handler_->pipelines[signal.pipeline_name].delay_us =
            signal.receive_timestamp_us - signal.sensor_timestamp_us;

        main_handler_->pipelines[signal.pipeline_name].frame_count++;
        main_handler_->pipelines[signal.pipeline_name].seq++;
        main_handler_->pipelines[signal.pipeline_name].online = true;
        main_handler_->pipelines[signal.pipeline_name]
            .latest_frame_timestamp_us = micros_now;
        if (reporter_) {
          std::vector<StatusMonitorReport> reports;
          StatusMonitorReport report;
          report.report_type = StatusMonitorReport::FRAME;
          report.seq = main_handler_->pipelines[signal.pipeline_name].seq;
          report.pipeline_name = signal.pipeline_name;
          report.sensor_timestamp_us = signal.sensor_timestamp_us;
          report.publish_timestamp_us = micros_now;
          reports.emplace_back(report);
          reporter_(reports);
        }
      }
    }
  }
  /* process warning queue*/
  while (!main_handler_->warning_queue.empty()) {
    /* pop warning signal */
    {
      std::lock_guard<std::mutex> lg(main_handler_->warning_queue_lock_);
      signal_warning = main_handler_->warning_queue.front();
      main_handler_->warning_queue.pop();

      main_handler_->pipelines[signal_warning.pipeline_name]
          .camera_status_set.insert(signal_warning);
    }
  }

  /* calculate and generate report */
  auto time_diff = (micros_now - main_handler_->last_report_time);
  bool timestamp_rollback = false;
  bool regular_report = false;
  if (time_diff < 0) {
    main_handler_->last_report_time = micros_now;
    timestamp_rollback = true;
  } else if (time_diff >= 100 * 1000) {
    regular_report = true;
    main_handler_->last_report_time = micros_now;
  }
  std::vector<StatusMonitorReport> reports;

  std::map<std::string, PipelineHandler>::iterator iter;
  std::lock_guard<std::mutex> lg(main_handler_->pipelines_lock_);
  iter = main_handler_->pipelines.begin();
  while (iter != main_handler_->pipelines.end()) {
    if (timestamp_rollback) {
      /* systemtime time rollback warning */
      /* refresh start time */
      iter->second.pipeline_start_time_us =
          iter->second.frame_count_start_time_us = micros_now;
      if (reporter_) {
        StatusMonitorReport report;
        report.report_type = StatusMonitorReport::WARNING;
        report.pipeline_name = iter->first;
        report.width = iter->second.meta.width;
        report.height = iter->second.meta.height;
        report.bitrate = precision(iter->second.meta.bitrate, 2);
        report.warning = SM_TIMESTAMP_ROLLBACK;
        report.receive_timestamp_us = micros_now;
        report.publish_timestamp_us = micros_now;
        reports.emplace_back(report);
      }
      iter->second.last_report_time = micros_now;
    } else {
      auto frame_diff = (micros_now - iter->second.latest_frame_timestamp_us);
      if (frame_diff >= (2 * 1000 * (1000 / iter->second.meta.fps))) {
        /* frame loss warning */
        iter->second.online = false;
        if (reporter_) {
          StatusMonitorReport report;
          report.report_type = StatusMonitorReport::WARNING;
          report.pipeline_name = iter->first;
          report.width = iter->second.meta.width;
          report.height = iter->second.meta.height;
          report.bitrate = precision(iter->second.meta.bitrate, 2);
          report.warning = SM_FRAME_LOSS;
          report.receive_timestamp_us = micros_now;
          report.publish_timestamp_us = micros_now;
          reports.emplace_back(report);
        }
        iter->second.latest_frame_timestamp_us = micros_now;
      }
      if (regular_report) {
        if (reporter_) {
          StatusMonitorReport report;
          report.report_type = StatusMonitorReport::HEART_BEAT;
          report.pipeline_name = iter->first;
          if (iter->second.seq > 0) {
            report.fps =
                (float)iter->second.frame_count /
                ((micros_now - iter->second.frame_count_start_time_us) /
                 1000000);

          } else {
            report.fps = 0.00;
          }
          report.width = iter->second.meta.width;
          report.height = iter->second.meta.height;
          report.bitrate = precision(iter->second.meta.bitrate, 2);
          report.publish_timestamp_us = micros_now;
          uint32_t logical_frames_total = 0;
          if (iter->second.pipeline_start_time_us > 0) {
            logical_frames_total = round(
                (iter->second.meta.fps *
                 ((float)(micros_now - iter->second.pipeline_start_time_us) /
                  1000000)));
          }
          uint32_t losted_frames =
              fabs(round((logical_frames_total - iter->second.seq)));
          if (iter->second.seq > 0) {
            if (losted_frames > 0) {
              report.frame_loss = std::to_string(losted_frames) + "/" +
                                  std::to_string(logical_frames_total);
            } else {
              report.frame_loss = std::to_string(0) + "/" +
                                  std::to_string(logical_frames_total);
            }
          } else {
            report.frame_loss = std::to_string(logical_frames_total) + "/" +
                                std::to_string(logical_frames_total);
          }
          report.online = iter->second.online;
          report.sync = iter->second.sync;
          report.delay_us = iter->second.delay_us;
          reports.emplace_back(report);

          if ((micros_now - iter->second.frame_count_start_time_us) >
              1000 * 1000 * 60) {
            /* reset fps calculate duration */
            iter->second.frame_count = 0;
            iter->second.frame_count_start_time_us = micros_now;
          }
          /* set status in reports*/
          if (iter->second.camera_status_set.size() > 0) {
            for (auto it = iter->second.camera_status_set.begin();
                 it != iter->second.camera_status_set.end();) {
              StatusMonitorReport report;
              if (it->warning == SM_STATUS_OK) {
                iter->second.camera_status_set.clear();
                report.report_type = StatusMonitorReport::WARNING;
                report.pipeline_name = it->pipeline_name;
                report.receive_timestamp_us = it->timestamp_us;
                report.publish_timestamp_us = micros_now;
                report.warning = it->warning;
                report.width = iter->second.meta.width;
                report.height = iter->second.meta.height;
                reports.emplace_back(report);
                break;
              } else if (it->warning == SM_ENCODE_ERROR) {
                report.report_type = StatusMonitorReport::WARNING;
                report.pipeline_name = it->pipeline_name;
                report.receive_timestamp_us = it->timestamp_us;
                report.publish_timestamp_us = micros_now;
                report.warning = it->warning;
                report.width = iter->second.meta.width;
                report.height = iter->second.meta.height;
                reports.emplace_back(report);
                iter->second.camera_status_set.erase(it++);
              } else {
                report.report_type = StatusMonitorReport::WARNING;
                report.pipeline_name = it->pipeline_name;
                report.receive_timestamp_us = it->timestamp_us;
                report.publish_timestamp_us = micros_now;
                report.warning = it->warning;
                report.width = iter->second.meta.width;
                report.height = iter->second.meta.height;
                reports.emplace_back(report);
                ++it;
              }
            }
          }
        }
        iter->second.last_report_time = micros_now;
      }
    }
    iter++;
  }
  if (reporter_ && reports.size() > 0) {
    reporter_(reports);
  }

  return;
};

void StatusMonitor::runner() {
  while (!main_handler_->is_quit) {
    usleep(100);
    run_once();
  }
  printf("status monitor runner quit");
};

void StatusMonitor::run_forever() {
  main_handler_->running_thread =
      std::make_shared<std::thread>(&StatusMonitor::runner, this);
  pthread_setname_np(main_handler_->running_thread->native_handle(),
                     "status_monitor_thread");
};

void StatusMonitor::stop() {
  main_handler_->is_quit = true;
  if (main_handler_->running_thread->joinable()) {
    main_handler_->running_thread->join();
  } else {
    printf("fail to stop status monitor runner");
  }
  return;
};

void StatusMonitor::pipeline_registration(PipelineInformation meta) {
  if ((meta.fps <= 0) || (meta.fps > 30)) {
    printf("pipeline registration with illigal fps %d, set to default 10",
           meta.fps);
    meta.fps = 10;
  }

  std::lock_guard<std::mutex> lg(main_handler_->pipelines_lock_);
  if (main_handler_->pipelines.find(meta.pipeline_name) !=
      main_handler_->pipelines.end()) {
    main_handler_->pipelines[meta.pipeline_name].meta = meta;
  } else {
    PipelineHandler new_handler;
    new_handler.meta = meta;
    main_handler_->pipelines.insert(
        std::make_pair(meta.pipeline_name, new_handler));
  }

  return;
};

void StatusMonitor::signal(StatusMonitorFrame signal) {
  {
    std::lock_guard<std::mutex> lg(main_handler_->signal_queue_lock_);
    main_handler_->frame_queue.push(signal);
  }
  return;
};

void StatusMonitor::signal(StatusSignalWarning signal) {
  {
    std::lock_guard<std::mutex> lg(main_handler_->warning_queue_lock_);
    StatusMonitorWarning status_warnings;
    Conv_Signal2Status(signal, status_warnings);
    main_handler_->warning_queue.push(status_warnings);
  }
  return;
};
void StatusMonitor::set_reporter_callback(
    StatusMonitorReporterCallback callback) {
  reporter_ = callback;
  return;
};

StatusMonitor &StatusMonitor::operator=(const StatusMonitor &) {
  return *this;
};

StatusMonitor &StatusMonitor::getInstance() {
  static StatusMonitor instance;
  return instance;
};

float StatusMonitor::precision(float f, int places) {
  float n = std::pow(10.0f, places);
  return std::round(f * n) / n;
}

void StatusMonitor::Conv_Signal2Status(StatusSignalWarning &signal,
                                       StatusMonitorWarning &warning) {
  StatusMonitorWarning monitor_warnings;
  warning.timestamp_us = signal.timestamp_us;
  warning.pipeline_name = signal.pipeline_name;
  if (CAMERA_SERVICE_MONITOR_STATUS_MAP.find(signal.camera_status) ==
      CAMERA_SERVICE_MONITOR_STATUS_MAP.end()) {
    printf("camera name %s  status %s not found in CAMERA_SERVICE_STATUS_MAP\n",
           signal.pipeline_name.c_str(), signal.camera_status.c_str());
  } else {
    warning.warning =
        CAMERA_SERVICE_MONITOR_STATUS_MAP.at(signal.camera_status);
  }
}
} // namespace CameraService
