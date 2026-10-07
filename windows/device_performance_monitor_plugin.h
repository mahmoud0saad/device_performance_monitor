#ifndef FLUTTER_PLUGIN_DEVICE_PERFORMANCE_MONITOR_PLUGIN_H_
#define FLUTTER_PLUGIN_DEVICE_PERFORMANCE_MONITOR_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include <memory>

namespace device_performance_monitor {

class DevicePerformanceMonitorPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  DevicePerformanceMonitorPlugin();

  virtual ~DevicePerformanceMonitorPlugin();

  // Disallow copy and assign.
  DevicePerformanceMonitorPlugin(const DevicePerformanceMonitorPlugin&) = delete;
  DevicePerformanceMonitorPlugin& operator=(const DevicePerformanceMonitorPlugin&) = delete;

  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

}  // namespace device_performance_monitor

#endif  // FLUTTER_PLUGIN_DEVICE_PERFORMANCE_MONITOR_PLUGIN_H_
