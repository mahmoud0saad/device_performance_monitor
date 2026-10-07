#include "include/device_performance_monitor/device_performance_monitor_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

#include <cstring>

#include "performance_utils.h"
#include "device_performance_monitor_plugin_private.h"

#define DEVICE_PERFORMANCE_MONITOR_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), device_performance_monitor_plugin_get_type(), \
                              DevicePerformanceMonitorPlugin))

struct _DevicePerformanceMonitorPlugin {
  GObject parent_instance;
};

G_DEFINE_TYPE(DevicePerformanceMonitorPlugin, device_performance_monitor_plugin,
              g_object_get_type())

static FlMethodResponse* make_metrics_response(
    const device_performance_monitor::PerformanceMetrics& metrics) {
  try {
    g_autoptr(FlValue) map = fl_value_new_map();
    fl_value_set_string_take(map, "cpu",
                             fl_value_new_string(metrics.cpu.c_str()));
    fl_value_set_string_take(map, "ram",
                             fl_value_new_string(metrics.ram.c_str()));
    fl_value_set_string_take(map, "temperature",
                             fl_value_new_string(metrics.temperature.c_str()));
    return FL_METHOD_RESPONSE(fl_method_success_response_new(map));
  } catch (...) {
    g_autoptr(FlValue) map = fl_value_new_map();
    fl_value_set_string_take(map, "cpu", fl_value_new_string("N/A"));
    fl_value_set_string_take(map, "ram", fl_value_new_string("N/A"));
    fl_value_set_string_take(map, "temperature", fl_value_new_string("N/A"));
    return FL_METHOD_RESPONSE(fl_method_success_response_new(map));
  }
}

FlMethodResponse* get_performance_metrics_response() {
  try {
    const auto metrics =
        device_performance_monitor::CollectPerformanceMetrics();
    return make_metrics_response(metrics);
  } catch (...) {
    device_performance_monitor::PerformanceMetrics na;
    return make_metrics_response(na);
  }
}

static void device_performance_monitor_plugin_handle_method_call(
    DevicePerformanceMonitorPlugin* self,
    FlMethodCall* method_call) {
  g_autoptr(FlMethodResponse) response = nullptr;

  try {
    const gchar* method = fl_method_call_get_name(method_call);
    if (strcmp(method, "getPerformanceMetrics") == 0) {
      response = get_performance_metrics_response();
    } else {
      // Unknown methods still return the stable map.
      device_performance_monitor::PerformanceMetrics na;
      response = make_metrics_response(na);
    }
  } catch (...) {
    device_performance_monitor::PerformanceMetrics na;
    response = make_metrics_response(na);
  }

  fl_method_call_respond(method_call, response, nullptr);
}

static void device_performance_monitor_plugin_dispose(GObject* object) {
  G_OBJECT_CLASS(device_performance_monitor_plugin_parent_class)->dispose(object);
}

static void device_performance_monitor_plugin_class_init(
    DevicePerformanceMonitorPluginClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = device_performance_monitor_plugin_dispose;
}

static void device_performance_monitor_plugin_init(
    DevicePerformanceMonitorPlugin* self) {}

static void method_call_cb(FlMethodChannel* channel, FlMethodCall* method_call,
                           gpointer user_data) {
  DevicePerformanceMonitorPlugin* plugin =
      DEVICE_PERFORMANCE_MONITOR_PLUGIN(user_data);
  device_performance_monitor_plugin_handle_method_call(plugin, method_call);
}

void device_performance_monitor_plugin_register_with_registrar(
    FlPluginRegistrar* registrar) {
  try {
    DevicePerformanceMonitorPlugin* plugin = DEVICE_PERFORMANCE_MONITOR_PLUGIN(
        g_object_new(device_performance_monitor_plugin_get_type(), nullptr));

    g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
    g_autoptr(FlMethodChannel) channel = fl_method_channel_new(
        fl_plugin_registrar_get_messenger(registrar),
        "device_performance_monitor", FL_METHOD_CODEC(codec));
    fl_method_channel_set_method_call_handler(channel, method_call_cb,
                                              g_object_ref(plugin),
                                              g_object_unref);

    g_object_unref(plugin);
  } catch (...) {
    // Registration failure must not crash the host.
  }
}
