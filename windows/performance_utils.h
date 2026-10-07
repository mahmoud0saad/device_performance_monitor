#ifndef DEVICE_PERFORMANCE_MONITOR_UTILS_H_
#define DEVICE_PERFORMANCE_MONITOR_UTILS_H_

#include <string>

namespace device_performance_monitor {

struct PerformanceMetrics {
  std::string cpu = "N/A";
  std::string ram = "N/A";
  std::string temperature = "N/A";
};

// Never throws. Always returns the three string fields.
PerformanceMetrics CollectPerformanceMetrics();

}  // namespace device_performance_monitor

#endif  // DEVICE_PERFORMANCE_MONITOR_UTILS_H_
