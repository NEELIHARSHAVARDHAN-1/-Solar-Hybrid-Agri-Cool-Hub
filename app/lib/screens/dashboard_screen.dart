import 'package:flutter/material.dart';
import 'package:intl/intl.dart';
import '../models/models.dart';
import '../services/api_service.dart';

class DashboardScreen extends StatefulWidget {
  final ApiService api;
  final String deviceId;

  const DashboardScreen(
      {super.key, required this.api, required this.deviceId});

  @override
  State<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> {
  DeviceStatus? _status;
  bool _loading = true;
  String? _error;

  @override
  void initState() {
    super.initState();
    _load();
  }

  Future<void> _load() async {
    setState(() {
      _loading = true;
      _error = null;
    });
    try {
      final s = await widget.api.getDeviceStatus(widget.deviceId);
      setState(() {
        _status = s;
        _loading = false;
      });
    } catch (e) {
      setState(() {
        _error = e.toString();
        _loading = false;
      });
    }
  }

  bool _isStale(DateTime? lastSeen) {
    if (lastSeen == null) return true;
    return DateTime.now().difference(lastSeen).inMinutes >= 5;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Dashboard'),
        actions: [
          IconButton(icon: const Icon(Icons.refresh), onPressed: _load)
        ],
      ),
      body: _loading
          ? const Center(child: CircularProgressIndicator())
          : _error != null
              ? _buildError()
              : _buildContent(),
    );
  }

  Widget _buildError() {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Icon(Icons.error_outline, size: 48, color: Colors.red),
            const SizedBox(height: 12),
            Text('Failed to load data', style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 8),
            Text(_error!, textAlign: TextAlign.center,
                style: const TextStyle(color: Colors.grey)),
            const SizedBox(height: 16),
            ElevatedButton(onPressed: _load, child: const Text('Retry')),
          ],
        ),
      ),
    );
  }

  Widget _buildContent() {
    final status = _status!;
    final latest = status.latest;
    final stale  = _isStale(status.lastSeen);

    return RefreshIndicator(
      onRefresh: _load,
      child: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          // Device status card
          _StatusCard(
            deviceId: status.deviceId,
            isOnline: status.isOnline,
            lastSeen: status.lastSeen,
          ),
          if (stale && !status.isOnline)
            Container(
              margin: const EdgeInsets.only(top: 8),
              padding: const EdgeInsets.all(10),
              decoration: BoxDecoration(
                color: Colors.orange.shade100,
                borderRadius: BorderRadius.circular(8),
                border: Border.all(color: Colors.orange),
              ),
              child: const Row(children: [
                Icon(Icons.warning_amber, color: Colors.orange),
                SizedBox(width: 8),
                Expanded(
                    child: Text('Device is offline or readings are stale',
                        style: TextStyle(color: Colors.orange))),
              ]),
            ),
          const SizedBox(height: 16),
          if (latest == null)
            const Center(child: Text('No readings available yet.'))
          else ...[
            _SensorCard(
              title: 'Temperature',
              value: latest.temperature != null
                  ? '${latest.temperature!.toStringAsFixed(1)} °C'
                  : 'N/A',
              icon: Icons.thermostat,
              color: Colors.orange,
            ),
            _SensorCard(
              title: 'Humidity',
              value: latest.humidity != null
                  ? '${latest.humidity!.toStringAsFixed(1)} %'
                  : 'N/A',
              icon: Icons.water_drop,
              color: Colors.blue,
            ),
            _SensorCard(
              title: 'MQ-3 (raw ADC)',
              value: '${latest.mq3.toStringAsFixed(2)} V',
              icon: Icons.science,
              color: Colors.purple,
            ),
            _SensorCard(
              title: 'MQ-135 (raw ADC)',
              value:  '${latest.mq135.toStringAsFixed(2)} V',
              icon: Icons.science,
              color: Colors.teal,
            ),
            _SensorCard(
              title: 'MQ-4 (raw ADC)',
              value: '${latest.mq4.toStringAsFixed(2)} V',
              icon: Icons.science,
              color: Colors.green,
            ),
            if (latest.batteryPercent != null)
              _SensorCard(
                title: 'Battery',
                value: '${latest.batteryPercent} %',
                icon: Icons.battery_4_bar,
                color: latest.batteryPercent! > 20
                    ? Colors.green
                    : Colors.red,
              ),
            Padding(
              padding: const EdgeInsets.only(top: 4),
              child: Text(
                'Last reading: ${DateFormat('dd MMM yyyy HH:mm:ss').format(latest.timestamp)}',
                textAlign: TextAlign.center,
                style: const TextStyle(color: Colors.grey, fontSize: 12),
              ),
            ),
          ],
        ],
      ),
    );
  }
}

class _StatusCard extends StatelessWidget {
  final String deviceId;
  final bool isOnline;
  final DateTime? lastSeen;

  const _StatusCard(
      {required this.deviceId, required this.isOnline, this.lastSeen});

  @override
  Widget build(BuildContext context) {
    return Card(
      child: ListTile(
        leading: Icon(
          isOnline ? Icons.wifi : Icons.wifi_off,
          color: isOnline ? Colors.green : Colors.red,
          size: 32,
        ),
        title: Text(deviceId,
            style: const TextStyle(fontWeight: FontWeight.bold)),
        subtitle: Text(isOnline
            ? 'Online'
            : lastSeen != null
                ? 'Offline — last seen ${DateFormat('HH:mm dd MMM').format(lastSeen!)}'
                : 'Offline — never connected'),
        trailing: Container(
          padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
          decoration: BoxDecoration(
            color: isOnline ? Colors.green : Colors.red,
            borderRadius: BorderRadius.circular(12),
          ),
          child: Text(
            isOnline ? 'ONLINE' : 'OFFLINE',
            style: const TextStyle(
                color: Colors.white,
                fontWeight: FontWeight.bold,
                fontSize: 11),
          ),
        ),
      ),
    );
  }
}

class _SensorCard extends StatelessWidget {
  final String title;
  final String value;
  final IconData icon;
  final Color color;

  const _SensorCard(
      {required this.title,
      required this.value,
      required this.icon,
      required this.color});

  @override
  Widget build(BuildContext context) {
    return Card(
      margin: const EdgeInsets.symmetric(vertical: 6),
      child: ListTile(
        leading: Icon(icon, color: color),
        title: Text(title),
        trailing: Text(value,
            style: TextStyle(
                fontWeight: FontWeight.bold, fontSize: 18, color: color)),
      ),
    );
  }
}
