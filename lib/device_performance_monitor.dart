import 'device_performance.dart';
import 'device_performance_monitor_platform_interface.dart';

export 'device_performance.dart';

/// Reads CPU, RAM, and temperature. Never throws.
///
/// Success and failure always return the same three string fields.
Future<DevicePerformance> getDevicePerformance() async {
  try {
    final result =
        await DevicePerformanceMonitorPlatform.instance.getDevicePerformance();
    return DevicePerformance(
      cpuUsage: result.cpuUsage.isEmpty ? 'N/A' : result.cpuUsage,
      ramUsage: result.ramUsage.isEmpty ? 'N/A' : result.ramUsage,
      temperature: result.temperature.isEmpty ? 'N/A' : result.temperature,
    );
  } catch (_) {
    return DevicePerformance.unavailable;
  }
}

/// Convenience wrapper around [getDevicePerformance].
class DevicePerformanceMonitor {
  Future<DevicePerformance> getMetrics() => getDevicePerformance();
}
