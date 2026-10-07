//
//  Generated file. Do not edit.
//

// clang-format off

#include "generated_plugin_registrant.h"

#include <device_performance_monitor/device_performance_monitor_plugin.h>

void fl_register_plugins(FlPluginRegistry* registry) {
  g_autoptr(FlPluginRegistrar) device_performance_monitor_registrar =
      fl_plugin_registry_get_registrar_for_plugin(registry, "DevicePerformanceMonitorPlugin");
  device_performance_monitor_plugin_register_with_registrar(device_performance_monitor_registrar);
}
