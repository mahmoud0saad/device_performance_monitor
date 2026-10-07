import 'package:flutter_test/flutter_test.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';
import 'package:device_performance_monitor/device_performance_monitor.dart';
import 'package:device_performance_monitor/device_performance_monitor_platform_interface.dart';

class _MockPlatform
    with MockPlatformInterfaceMixin
    implements DevicePerformanceMonitorPlatform {
  _MockPlatform(this.next);

  DevicePerformance next;
  bool throwOnCall = false;

  @override
  Future<DevicePerformance> getDevicePerformance() async {
    if (throwOnCall) {
      throw StateError('boom');
    }
    return next;
  }
}

class _ThrowingPlatform
    with MockPlatformInterfaceMixin
    implements DevicePerformanceMonitorPlatform {
  @override
  Future<DevicePerformance> getDevicePerformance() {
    throw StateError('platform exploded');
  }
}

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  group('DevicePerformance.fromMap', () {
    test('maps valid values', () {
      final metrics = DevicePerformance.fromMap({
        'cpu': '42%',
        'ram': '67.23%',
        'temperature': '45.0°C',
      });
      expect(metrics.cpuUsage, '42%');
      expect(metrics.ramUsage, '67.23%');
      expect(metrics.temperature, '45.0°C');
    });

    test('null map becomes N/A for all fields', () {
      final metrics = DevicePerformance.fromMap(null);
      expect(metrics, DevicePerformance.unavailable);
    });

    test('missing keys become N/A without changing shape', () {
      final metrics = DevicePerformance.fromMap({'cpu': '10%'});
      expect(metrics.cpuUsage, '10%');
      expect(metrics.ramUsage, 'N/A');
      expect(metrics.temperature, 'N/A');
    });
  });

  group('getDevicePerformance', () {
    tearDown(() {
      DevicePerformanceMonitorPlatform.instance =
          _MockPlatform(DevicePerformance.unavailable);
    });

    test('returns platform values', () async {
      DevicePerformanceMonitorPlatform.instance = _MockPlatform(
        const DevicePerformance(
          cpuUsage: '12%',
          ramUsage: '34.56%',
          temperature: '40.0°C',
        ),
      );

      final metrics = await getDevicePerformance();
      expect(metrics.cpuUsage, '12%');
      expect(metrics.ramUsage, '34.56%');
      expect(metrics.temperature, '40.0°C');
    });

    test('platform throw becomes unavailable without rethrowing', () async {
      DevicePerformanceMonitorPlatform.instance = _ThrowingPlatform();

      final metrics = await getDevicePerformance();
      expect(metrics, DevicePerformance.unavailable);
    });

    test('empty strings are normalized to N/A', () async {
      DevicePerformanceMonitorPlatform.instance = _MockPlatform(
        const DevicePerformance(
          cpuUsage: '',
          ramUsage: '',
          temperature: '',
        ),
      );

      final metrics = await getDevicePerformance();
      expect(metrics, DevicePerformance.unavailable);
    });
  });
}
