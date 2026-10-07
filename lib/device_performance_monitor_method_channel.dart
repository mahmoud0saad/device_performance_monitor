import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'device_performance.dart';
import 'device_performance_monitor_platform_interface.dart';

/// Method-channel implementation. Never throws to callers.
class MethodChannelDevicePerformanceMonitor extends DevicePerformanceMonitorPlatform {
  @visibleForTesting
  final methodChannel = const MethodChannel('device_performance_monitor');

  @override
  Future<DevicePerformance> getDevicePerformance() async {
    try {
      if (kIsWeb) {
        return DevicePerformance.unavailable;
      }
      switch (defaultTargetPlatform) {
        case TargetPlatform.android:
        case TargetPlatform.windows:
        case TargetPlatform.linux:
          break;
        default:
          return DevicePerformance.unavailable;
      }

      final raw = await methodChannel.invokeMethod<dynamic>(
        'getPerformanceMetrics',
      );

      if (raw is Map) {
        return DevicePerformance.fromMap(Map<Object?, Object?>.from(raw));
      }
      return DevicePerformance.unavailable;
    } catch (_) {
      return DevicePerformance.unavailable;
    }
  }
}
