/// Stable performance snapshot used by MQTT device status and callers.
///
/// Fields are always non-null strings. Failures use `"N/A"`.
class DevicePerformance {
  /// System CPU such as `"42%"`, app CPU such as `"3.5% (App)"`, or `"N/A"`.
  final String cpuUsage;

  /// Used RAM percent such as `"67.23%"`, or `"N/A"`.
  final String ramUsage;

  /// Device temperature such as `"45.0°C"`, or `"N/A"`.
  final String temperature;

  const DevicePerformance({
    required this.cpuUsage,
    required this.ramUsage,
    required this.temperature,
  });

  /// Canonical failure / unsupported-platform value.
  static const DevicePerformance unavailable = DevicePerformance(
    cpuUsage: 'N/A',
    ramUsage: 'N/A',
    temperature: 'N/A',
  );

  /// Builds a [DevicePerformance] from a platform channel map.
  ///
  /// Missing or invalid entries become `"N/A"`. Never throws.
  factory DevicePerformance.fromMap(Map<Object?, Object?>? map) {
    try {
      if (map == null) {
        return DevicePerformance.unavailable;
      }
      return DevicePerformance(
        cpuUsage: _stringOrNa(map['cpu']),
        ramUsage: _stringOrNa(map['ram']),
        temperature: _stringOrNa(map['temperature']),
      );
    } catch (_) {
      return DevicePerformance.unavailable;
    }
  }

  Map<String, String> toMap() => {
        'cpu': cpuUsage,
        'ram': ramUsage,
        'temperature': temperature,
      };

  @override
  String toString() =>
      'DevicePerformance(cpuUsage: $cpuUsage, ramUsage: $ramUsage, temperature: $temperature)';

  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      other is DevicePerformance &&
          cpuUsage == other.cpuUsage &&
          ramUsage == other.ramUsage &&
          temperature == other.temperature;

  @override
  int get hashCode => Object.hash(cpuUsage, ramUsage, temperature);

  static String _stringOrNa(Object? value) {
    try {
      if (value == null) {
        return 'N/A';
      }
      final text = value.toString().trim();
      if (text.isEmpty) {
        return 'N/A';
      }
      return text;
    } catch (_) {
      return 'N/A';
    }
  }
}
