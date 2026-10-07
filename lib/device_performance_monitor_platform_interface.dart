import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'device_performance.dart';
import 'device_performance_monitor_method_channel.dart';

abstract class DevicePerformanceMonitorPlatform extends PlatformInterface {
  DevicePerformanceMonitorPlatform() : super(token: _token);

  static final Object _token = Object();

  static DevicePerformanceMonitorPlatform _instance =
      MethodChannelDevicePerformanceMonitor();

  static DevicePerformanceMonitorPlatform get instance => _instance;

  static set instance(DevicePerformanceMonitorPlatform instance) {
    PlatformInterface.verify(instance, _token);
    _instance = instance;
  }

  /// Must never throw. Return [DevicePerformance.unavailable] on failure.
  Future<DevicePerformance> getDevicePerformance() {
    return Future<DevicePerformance>.value(DevicePerformance.unavailable);
  }
}
