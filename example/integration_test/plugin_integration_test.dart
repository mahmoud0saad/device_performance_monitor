import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';
import 'package:device_performance_monitor/device_performance_monitor.dart';

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('getDevicePerformance returns stable string fields', (
    WidgetTester tester,
  ) async {
    final metrics = await getDevicePerformance();
    expect(metrics.cpuUsage.isNotEmpty, isTrue);
    expect(metrics.ramUsage.isNotEmpty, isTrue);
    expect(metrics.temperature.isNotEmpty, isTrue);
  });
}
