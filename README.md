# device_performance_monitor

Fail-safe CPU, RAM, and temperature metrics for Flutter apps (Android, Windows, Linux).

The public API **never throws**. Success and failure always return the same three string fields.

## Usage

```dart
import 'package:device_performance_monitor/device_performance_monitor.dart';

final metrics = await getDevicePerformance();
// Always present:
print(metrics.cpuUsage);    // "42%", "3.5% (App)", or "N/A"
print(metrics.ramUsage);    // "67.23%" or "N/A"
print(metrics.temperature); // "45.0°C" or "N/A"
```

## Result contract

| Field | Examples |
| --- | --- |
| `cpuUsage` | `"42%"`, `"3.5% (App)"`, `"N/A"` |
| `ramUsage` | `"67.23%"`, `"N/A"` |
| `temperature` | `"45.0°C"`, `"N/A"` |

Any platform, channel, or metric failure fills the failed field(s) with `"N/A"`. Keys and types never change.

## Platforms

| Platform | CPU | RAM | Temperature |
| --- | --- | --- | --- |
| Android | `top` / app process fallback | `ActivityManager` | thermal sysfs, then battery |
| Windows | `GetSystemTimes` | `GlobalMemoryStatusEx` | WMI thermal zone (best effort) |
| Linux | `/proc/stat` | `/proc/meminfo` | thermal sysfs |
| Other | `"N/A"` | `"N/A"` | `"N/A"` |

## License

MIT
