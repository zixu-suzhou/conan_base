/*****************************************************************************
 * Copyright (C) 2022 Momenta Technology Co., Ltd. All rights reserved.
 *
 * No.58 Qinglonggang Rd, Suzhou, Jiangsu, PR China, contact@momenta.ai
 *
 * https://www.momenta.cn/
 *
 * Filename         : status_monitor_base.h
 * Created          : 2022-09-05 19:28
 * Last modified    : 2022-09-05 19:28
 * Author           : denny.zhang <denny.zhang@momenta.ai>
 * Description      : abstract class of status monitor handler
 *****************************************************************************/

#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

namespace CameraService {
class StatusMonitorAbstract {
public:
  enum STATUS_MONITOR_SIGNALS {
    FRAME = 0,
    SERVICE,
    WARNING,
    ERROR,
    STATUS_MONITOR_SIGNAL_MAX
  };
  enum STATUS_MONITOR_WARNING {
    SM_STATUS_OK = 0,
    SM_LOCK_LOST,
    SM_DRIVER_ERROR,
    SM_ENCODE_ERROR,
    SM_FRAME_LOSS,
    SM_STATUS_DELAY,
    SM_HARDWARE_CHANGED,
    SM_CALIB_LOST,
    SM_INIT_FAIL,
    SM_SEDERS_LOCK,
    SM_TIMESTAMP_ROLLBACK,
    STATUS_MONITOR_WARNINGS_MAX = 99
  };

  struct PipelineInformation {
    std::string pipeline_name;
    uint8_t data_type = 0;
    enum : uint8_t { BGR888 = 0, H26x = 1, JPEG = 2 };
    uint32_t fps = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    float bitrate = 0;
  };

  struct StatusMonitorWarning {
    std::string pipeline_name;
    uint64_t timestamp_us;
    STATUS_MONITOR_WARNING warning;
    bool operator==(const StatusMonitorWarning &rhs) const {
      return (warning == rhs.warning) && (pipeline_name == rhs.pipeline_name);
    }
  };
  struct StatusSignalWarning {
    std::string pipeline_name;
    uint64_t timestamp_us;
    std::string camera_status;
  };
  struct StatusMonitorWarningHashFunc {
    size_t operator()(const StatusMonitorWarning &rhs) const {
      return std::hash<std::string>()(rhs.pipeline_name) ^
             std::hash<int>()(rhs.warning);
    }
  };

  struct StatusMonitorFrame {
    std::string pipeline_name;
    uint64_t sensor_timestamp_us;
    uint64_t receive_timestamp_us;
    uint64_t publish_timestamp_us;
  };

  struct StatusMonitorReport {
    uint8_t report_type;
    enum : uint8_t { FRAME = 0, HEART_BEAT, WARNING, ERROR };
    std::string pipeline_name;
    uint64_t seq;
    float fps;
    uint32_t width;
    uint32_t height;
    float bitrate;
    std::string frame_loss;
    uint64_t sensor_timestamp_us;
    uint64_t receive_timestamp_us;
    uint64_t publish_timestamp_us;
    STATUS_MONITOR_WARNING warning;
    bool online;
    bool sync;
    uint64_t delay_us;
    std::string details;
  };

  using StatusMonitorReporterCallback =
      std::function<void(const std::vector<StatusMonitorReport> &)>;

public:
  virtual void run_once() = 0;
  virtual void run_forever() = 0;
  virtual void stop() = 0;
  virtual void pipeline_registration(PipelineInformation meta) = 0;
  virtual void signal(StatusMonitorFrame frame) = 0;
  virtual void signal(StatusSignalWarning warning) = 0;
  virtual void
  set_reporter_callback(StatusMonitorReporterCallback callback) = 0;
};
} // namespace CameraService
