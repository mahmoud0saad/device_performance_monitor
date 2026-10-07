#include <flutter/method_call.h>
#include <flutter/method_result_functions.h>
#include <flutter/standard_method_codec.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "device_performance_monitor_plugin.h"

namespace device_performance_monitor {
namespace test {

namespace {

using flutter::EncodableMap;
using flutter::EncodableValue;
using flutter::MethodCall;
using flutter::MethodResultFunctions;

}  // namespace

TEST(DevicePerformanceMonitorPlugin, GetPerformanceMetricsReturnsStableMap) {
  DevicePerformanceMonitorPlugin plugin;
  EncodableMap result_map;

  plugin.HandleMethodCall(
      MethodCall("getPerformanceMetrics", std::make_unique<EncodableValue>()),
      std::make_unique<MethodResultFunctions<>>(
          [&result_map](const EncodableValue* result) {
            ASSERT_NE(result, nullptr);
            ASSERT_TRUE(std::holds_alternative<EncodableMap>(*result));
            result_map = std::get<EncodableMap>(*result);
          },
          nullptr, nullptr));

  EXPECT_TRUE(result_map.find(EncodableValue("cpu")) != result_map.end());
  EXPECT_TRUE(result_map.find(EncodableValue("ram")) != result_map.end());
  EXPECT_TRUE(result_map.find(EncodableValue("temperature")) !=
              result_map.end());
}

}  // namespace test
}  // namespace device_performance_monitor
