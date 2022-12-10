/*****************************************************************************
 * Copyright (C) 2022 Momenta Technology Co., Ltd. All rights reserved.
 *
 * No.58 Qinglonggang Rd, Suzhou, Jiangsu, PR China, contact@momenta.ai
 *
 * https://www.momenta.cn/
 *
 * Filename         : status_monitor.h
 * Created          : 2022-09-03 12:26
 * Last modified    : 2022-09-03 12:26
 * Author           : denny.zhang <denny.zhang@momenta.ai>
 * Description      :
 *****************************************************************************/
#pragma once
#include "status_monitor_base.h"

namespace CameraService {
class StatusMonitor : public StatusMonitorAbstract {
public:
  static StatusMonitor &getInstance();
  void run_once();
  void run_forever();
  void stop();
  void pipeline_registration(PipelineInformation meta);
  void signal(StatusMonitorFrame signal);
  void signal(StatusSignalWarning signal);
  void set_reporter_callback(StatusMonitorReporterCallback callback);

private:
  StatusMonitor();
  ~StatusMonitor();
  StatusMonitor(const StatusMonitor &);
  StatusMonitor &operator=(const StatusMonitor &);
  void runner();

  float precision(float f, int places);
  void Conv_Signal2Status(StatusSignalWarning &signal,
                          StatusMonitorWarning &warning);
  struct StatusMonitorMainHandler;

  StatusMonitorMainHandler *main_handler_ = nullptr;
  StatusMonitorReporterCallback reporter_ = nullptr;
};

} // namespace CameraService
