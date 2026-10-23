import 'dart:convert';
import 'package:http/http.dart' as http;
import '../models/models.dart';

class ApiService {
  final String baseUrl;

  ApiService(this.baseUrl);

  Uri _uri(String path, [Map<String, String>? params]) {
    final uri = Uri.parse('$baseUrl$path');
    if (params != null) {
      return uri.replace(queryParameters: params);
    }
    return uri;
  }

  Future<Map<String, dynamic>> _get(String path,
      [Map<String, String>? params]) async {
    final response =
        await http.get(_uri(path, params)).timeout(const Duration(seconds: 15));
    if (response.statusCode == 200) {
      return json.decode(response.body) as Map<String, dynamic>;
    }
    throw ApiException(response.statusCode, response.body);
  }

  // ---- Device ----------------------------------------------------------------

  Future<List<Map<String, dynamic>>> listDevices() async {
    final response = await http
        .get(_uri('/devices'))
        .timeout(const Duration(seconds: 15));
    if (response.statusCode == 200) {
      return List<Map<String, dynamic>>.from(json.decode(response.body) as List);
    }
    throw ApiException(response.statusCode, response.body);
  }

  Future<DeviceStatus> getDeviceStatus(String deviceId) async {
    final data = await _get('/devices/$deviceId/status');
    return DeviceStatus.fromJson(data);
  }

  // ---- Telemetry -------------------------------------------------------------

  Future<PaginatedTelemetry> getTelemetry(
    String deviceId, {
    int limit = 100,
    int offset = 0,
    DateTime? from,
    DateTime? to,
  }) async {
    final params = <String, String>{
      'limit': '$limit',
      'offset': '$offset',
    };
    if (from != null) params['from'] = from.toUtc().toIso8601String();
    if (to != null) params['to'] = to.toUtc().toIso8601String();
    final data = await _get('/devices/$deviceId/telemetry', params);
    return PaginatedTelemetry.fromJson(data);
  }

  Future<TelemetryReading> getLatestTelemetry(String deviceId) async {
    final data = await _get('/devices/$deviceId/telemetry/latest');
    return TelemetryReading.fromJson(data);
  }

  // ---- Alerts ----------------------------------------------------------------

  Future<PaginatedAlerts> getAlerts(String deviceId,
      {int limit = 50, int offset = 0}) async {
    final data = await _get(
        '/devices/$deviceId/alerts', {'limit': '$limit', 'offset': '$offset'});
    return PaginatedAlerts.fromJson(data);
  }

  Future<PaginatedAlerts> getActiveAlerts(String deviceId) async {
    final data = await _get('/devices/$deviceId/alerts/active');
    return PaginatedAlerts.fromJson(data);
  }

  Future<Alert> resolveAlert(int alertId) async {
    final response = await http
        .patch(_uri('/alerts/$alertId/resolve'))
        .timeout(const Duration(seconds: 15));
    if (response.statusCode == 200) {
      return Alert.fromJson(
          json.decode(response.body) as Map<String, dynamic>);
    }
    throw ApiException(response.statusCode, response.body);
  }

  // ---- Health ----------------------------------------------------------------

  Future<bool> checkHealth() async {
    try {
      final response = await http
          .get(_uri('/health'))
          .timeout(const Duration(seconds: 5));
      return response.statusCode == 200;
    } catch (_) {
      return false;
    }
  }
}

class ApiException implements Exception {
  final int statusCode;
  final String body;
  ApiException(this.statusCode, this.body);

  @override
  String toString() => 'ApiException($statusCode): $body';
}
