#include <flutter_linux/flutter_linux.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "include/device_performance_monitor/device_performance_monitor_plugin.h"
#include "device_performance_monitor_plugin_private.h"

namespace device_performance_monitor {
namespace test {

TEST(DevicePerformanceMonitorPlugin, GetPerformanceMetricsReturnsStableMap) {
  g_autoptr(FlMethodResponse) response = get_performance_metrics_response();
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(FL_IS_METHOD_SUCCESS_RESPONSE(response));
  FlValue* result = fl_method_success_response_get_result(
      FL_METHOD_SUCCESS_RESPONSE(response));
  ASSERT_EQ(fl_value_get_type(result), FL_VALUE_TYPE_MAP);
  EXPECT_NE(fl_value_lookup_string(result, "cpu"), nullptr);
  EXPECT_NE(fl_value_lookup_string(result, "ram"), nullptr);
  EXPECT_NE(fl_value_lookup_string(result, "temperature"), nullptr);
}

}  // namespace test
}  // namespace device_performance_monitor
