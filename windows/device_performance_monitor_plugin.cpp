#include "device_performance_monitor_plugin.h"

#include <windows.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>

#include "performance_utils.h"

namespace device_performance_monitor {

// static
void DevicePerformanceMonitorPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows* registrar) {
  try {
    auto channel =
        std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
            registrar->messenger(), "device_performance_monitor",
            &flutter::StandardMethodCodec::GetInstance());

    auto plugin = std::make_unique<DevicePerformanceMonitorPlugin>();

    channel->SetMethodCallHandler(
        [plugin_pointer = plugin.get()](const auto& call, auto result) {
          plugin_pointer->HandleMethodCall(call, std::move(result));
        });

    registrar->AddPlugin(std::move(plugin));
  } catch (...) {
    // Registration failure must not crash the host.
  }
}

DevicePerformanceMonitorPlugin::DevicePerformanceMonitorPlugin() {}

DevicePerformanceMonitorPlugin::~DevicePerformanceMonitorPlugin() {}

void DevicePerformanceMonitorPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  try {
    PerformanceMetrics metrics;
    if (method_call.method_name() == "getPerformanceMetrics") {
      metrics = CollectPerformanceMetrics();
    } else {
      // Unknown methods still return the stable map.
      metrics = PerformanceMetrics{};
    }

    flutter::EncodableMap map;
    map[flutter::EncodableValue("cpu")] =
        flutter::EncodableValue(metrics.cpu);
    map[flutter::EncodableValue("ram")] =
        flutter::EncodableValue(metrics.ram);
    map[flutter::EncodableValue("temperature")] =
        flutter::EncodableValue(metrics.temperature);
    result->Success(flutter::EncodableValue(map));
  } catch (...) {
    try {
      flutter::EncodableMap map;
      map[flutter::EncodableValue("cpu")] = flutter::EncodableValue("N/A");
      map[flutter::EncodableValue("ram")] = flutter::EncodableValue("N/A");
      map[flutter::EncodableValue("temperature")] =
          flutter::EncodableValue("N/A");
      result->Success(flutter::EncodableValue(map));
    } catch (...) {
      // Result may already be completed.
    }
  }
}

}  // namespace device_performance_monitor
