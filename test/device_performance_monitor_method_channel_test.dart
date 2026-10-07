import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:device_performance_monitor/device_performance_monitor.dart';
import 'package:device_performance_monitor/device_performance_monitor_method_channel.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  final platform = MethodChannelDevicePerformanceMonitor();
  final channel = platform.methodChannel;

  tearDown(() {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(channel, null);
  });

  test('parses success map', () async {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(channel, (MethodCall methodCall) async {
      expect(methodCall.method, 'getPerformanceMetrics');
      return <String, String>{
        'cpu': '42%',
        'ram': '67.23%',
        'temperature': '45.0°C',
      };
    });

    final metrics = await platform.getDevicePerformance();
    expect(metrics.cpuUsage, '42%');
    expect(metrics.ramUsage, '67.23%');
    expect(metrics.temperature, '45.0°C');
  });

  test('MissingPluginException becomes unavailable', () async {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(channel, (MethodCall methodCall) async {
      throw MissingPluginException('missing');
    });

    final metrics = await platform.getDevicePerformance();
    expect(metrics, DevicePerformance.unavailable);
  });

  test('non-map payload becomes unavailable', () async {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(channel, (MethodCall methodCall) async {
      return 'not-a-map';
    });

    final metrics = await platform.getDevicePerformance();
    expect(metrics, DevicePerformance.unavailable);
  });
}
