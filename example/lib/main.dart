import 'package:flutter/material.dart';
import 'package:device_performance_monitor/device_performance_monitor.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  DevicePerformance _metrics = DevicePerformance.unavailable;

  @override
  void initState() {
    super.initState();
    _loadMetrics();
  }

  Future<void> _loadMetrics() async {
    final metrics = await getDevicePerformance();
    // ignore: avoid_print
    print(
      'PERF_TEST cpu=${metrics.cpuUsage} ram=${metrics.ramUsage} '
      'temp=${metrics.temperature}',
    );
    if (!mounted) {
      return;
    }
    setState(() {
      _metrics = metrics;
    });
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(title: const Text('device_performance_monitor')),
        body: Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text('CPU: ${_metrics.cpuUsage}'),
              Text('RAM: ${_metrics.ramUsage}'),
              Text('Temperature: ${_metrics.temperature}'),
              const SizedBox(height: 16),
              ElevatedButton(
                onPressed: _loadMetrics,
                child: const Text('Refresh'),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
