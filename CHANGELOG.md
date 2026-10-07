## 0.1.0

* Initial release.
* Fail-safe `getDevicePerformance()` returning `cpuUsage`, `ramUsage`, and `temperature` strings.
* Android, Windows, and Linux native collectors.
* Failures always map to `"N/A"` without throwing.
