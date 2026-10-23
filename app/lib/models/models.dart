// Data models matching backend API response shapes

class DeviceStatus {
  final String deviceId;
  final bool isOnline;
  final DateTime? lastSeen;
  final TelemetryReading? latest;

  DeviceStatus({
    required this.deviceId,
    required this.isOnline,
    this.lastSeen,
    this.latest,
  });

  factory DeviceStatus.fromJson(Map<String, dynamic> json) {
    return DeviceStatus(
      deviceId: json['device_id'] as String,
      isOnline: json['is_online'] as bool,
      lastSeen: json['last_seen'] != null
          ? DateTime.parse(json['last_seen'] as String).toLocal()
          : null,
      latest: json['latest'] != null
          ? TelemetryReading.fromJson(json['latest'] as Map<String, dynamic>)
          : null,
    );
  }
}

class TelemetryReading {
  final int id;
  final String deviceId;
  final DateTime timestamp;
  final double? temperature;
  final double? humidity;
  final double? mq3;
  final double? mq135;
  final double? mq4;
  final int? batteryPercent;
  final double? latitude;
  final double? longitude;
  final DateTime receivedAt;

  TelemetryReading({
    required this.id,
    required this.deviceId,
    required this.timestamp,
    this.temperature,
    this.humidity,
    required this.mq3,
    required this.mq135,
    required this.mq4,
    this.batteryPercent,
    this.latitude,
    this.longitude,
    required this.receivedAt,
  });

  factory TelemetryReading.fromJson(Map<String, dynamic> json) {
    return TelemetryReading(
      id: json['id'] as int,
      deviceId: json['device_id'] as String,
      timestamp: DateTime.parse(json['timestamp'] as String).toLocal(),
      temperature: (json['temperature'] as num?)?.toDouble(),
      humidity: (json['humidity'] as num?)?.toDouble(),
      mq3:   (json['mq3']   as num).toDouble(),
      mq135: (json['mq135'] as num).toDouble(),
      mq4:   (json['mq4']   as num).toDouble(),
      batteryPercent: json['battery_percent'] as int?,
      latitude: (json['latitude'] as num?)?.toDouble(),
      longitude: (json['longitude'] as num?)?.toDouble(),
      receivedAt: DateTime.parse(json['received_at'] as String).toLocal(),
    );
  }
}

class PaginatedTelemetry {
  final int total;
  final List<TelemetryReading> items;

  PaginatedTelemetry({required this.total, required this.items});

  factory PaginatedTelemetry.fromJson(Map<String, dynamic> json) {
    return PaginatedTelemetry(
      total: json['total'] as int,
      items: (json['items'] as List)
          .map((e) => TelemetryReading.fromJson(e as Map<String, dynamic>))
          .toList(),
    );
  }
}

class Alert {
  final int id;
  final String deviceId;
  final DateTime timestamp;
  final String alertType;
  final double value;
  final double threshold;
  final DateTime? resolvedAt;
  final DateTime createdAt;

  Alert({
    required this.id,
    required this.deviceId,
    required this.timestamp,
    required this.alertType,
    required this.value,
    required this.threshold,
    this.resolvedAt,
    required this.createdAt,
  });

  bool get isActive => resolvedAt == null;

  factory Alert.fromJson(Map<String, dynamic> json) {
    return Alert(
      id: json['id'] as int,
      deviceId: json['device_id'] as String,
      timestamp: DateTime.parse(json['timestamp'] as String).toLocal(),
      alertType: json['alert_type'] as String,
      value: (json['value'] as num).toDouble(),
      threshold: (json['threshold'] as num).toDouble(),
      resolvedAt: json['resolved_at'] != null
          ? DateTime.parse(json['resolved_at'] as String).toLocal()
          : null,
      createdAt: DateTime.parse(json['created_at'] as String).toLocal(),
    );
  }
}

class PaginatedAlerts {
  final int total;
  final List<Alert> items;

  PaginatedAlerts({required this.total, required this.items});

  factory PaginatedAlerts.fromJson(Map<String, dynamic> json) {
    return PaginatedAlerts(
      total: json['total'] as int,
      items: (json['items'] as List)
          .map((e) => Alert.fromJson(e as Map<String, dynamic>))
          .toList(),
    );
  }
}
