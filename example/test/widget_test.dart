import 'package:flutter_test/flutter_test.dart';
import 'package:device_performance_monitor/device_performance_monitor.dart';
import 'package:device_performance_monitor/device_performance_monitor_platform_interface.dart';
import 'package:device_performance_monitor_example/main.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class _FakePlatform
    with MockPlatformInterfaceMixin
    implements DevicePerformanceMonitorPlatform {
  @override
  Future<DevicePerformance> getDevicePerformance() async {
    return const DevicePerformance(
      cpuUsage: '10%',
      ramUsage: '20.00%',
      temperature: '30.0°C',
    );
  }
}

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  setUp(() {
    DevicePerformanceMonitorPlatform.instance = _FakePlatform();
  });

  testWidgets('shows performance metrics', (WidgetTester tester) async {
    await tester.pumpWidget(const MyApp());
    await tester.pumpAndSettle();

    expect(find.text('CPU: 10%'), findsOneWidget);
    expect(find.text('RAM: 20.00%'), findsOneWidget);
    expect(find.text('Temperature: 30.0°C'), findsOneWidget);
  });
}
