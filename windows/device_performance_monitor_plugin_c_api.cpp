#include "device_performance_monitor_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "device_performance_monitor_plugin.h"

void DevicePerformanceMonitorPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  device_performance_monitor::DevicePerformanceMonitorPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
