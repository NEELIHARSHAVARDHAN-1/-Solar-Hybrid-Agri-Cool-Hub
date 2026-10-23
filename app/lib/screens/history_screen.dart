import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';
import 'package:intl/intl.dart';
import '../models/models.dart';
import '../services/api_service.dart';

class HistoryScreen extends StatefulWidget {
  final ApiService api;
  final String deviceId;

  const HistoryScreen({super.key, required this.api, required this.deviceId});

  @override
  State<HistoryScreen> createState() => _HistoryScreenState();
}

class _HistoryScreenState extends State<HistoryScreen> {
  List<TelemetryReading> _readings = [];
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
      final result = await widget.api.getTelemetry(
        widget.deviceId,
        limit: 100,
        from: DateTime.now().subtract(const Duration(hours: 24)),
      );
      setState(() {
        _readings = result.items.reversed.toList(); // oldest first for chart
        _loading = false;
      });
    } catch (e) {
      setState(() {
        _error = e.toString();
        _loading = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('History (last 24 h)'),
        actions: [
          IconButton(icon: const Icon(Icons.refresh), onPressed: _load)
        ],
      ),
      body: _loading
          ? const Center(child: CircularProgressIndicator())
          : _error != null
              ? _buildError()
              : _readings.isEmpty
                  ? const Center(child: Text('No history available.'))
                  : _buildContent(),
    );
  }

  Widget _buildError() {
    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          const Icon(Icons.error_outline, size: 48, color: Colors.red),
          const SizedBox(height: 12),
          Text(_error!, textAlign: TextAlign.center,
              style: const TextStyle(color: Colors.grey)),
          const SizedBox(height: 16),
          ElevatedButton(onPressed: _load, child: const Text('Retry')),
        ],
      ),
    );
  }

  Widget _buildContent() {
    final tempSpots = <FlSpot>[];
    final humSpots  = <FlSpot>[];

    for (int i = 0; i < _readings.length; i++) {
      final r = _readings[i];
      final x = i.toDouble();
      if (r.temperature != null) tempSpots.add(FlSpot(x, r.temperature!));
      if (r.humidity    != null) humSpots.add(FlSpot(x, r.humidity!));
    }

    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        const Text('Temperature (°C)',
            style: TextStyle(fontWeight: FontWeight.bold)),
        const SizedBox(height: 8),
        SizedBox(
          height: 180,
          child: _buildLineChart(
              tempSpots, Colors.orange, 'Temp °C', 0, 50),
        ),
        const SizedBox(height: 24),
        const Text('Humidity (%)',
            style: TextStyle(fontWeight: FontWeight.bold)),
        const SizedBox(height: 8),
        SizedBox(
          height: 180,
          child: _buildLineChart(
              humSpots, Colors.blue, 'Humidity %', 0, 100),
        ),
        const SizedBox(height: 24),
        const Text('Recent Readings',
            style: TextStyle(fontWeight: FontWeight.bold)),
        const SizedBox(height: 8),
        ..._readings.reversed.take(20).map(_buildRow),
      ],
    );
  }

  Widget _buildLineChart(List<FlSpot> spots, Color color, String label,
      double minY, double maxY) {
    if (spots.isEmpty) {
      return const Center(child: Text('No data'));
    }
    return LineChart(
      LineChartData(
        minY: minY,
        maxY: maxY,
        gridData: const FlGridData(show: true),
        titlesData: FlTitlesData(
          bottomTitles: AxisTitles(
            sideTitles: SideTitles(
              showTitles: true,
              interval: (spots.length / 4).ceilToDouble().clamp(1, 9999),
              getTitlesWidget: (value, meta) {
                final idx = value.toInt();
                if (idx < 0 || idx >= _readings.length) {
                  return const SizedBox();
                }
                return Text(
                  DateFormat('HH:mm').format(_readings[idx].timestamp),
                  style: const TextStyle(fontSize: 9),
                );
              },
            ),
          ),
          leftTitles: AxisTitles(
            sideTitles: SideTitles(
              showTitles: true,
              reservedSize: 36,
              getTitlesWidget: (v, m) =>
                  Text(v.toStringAsFixed(0), style: const TextStyle(fontSize: 9)),
            ),
          ),
          topTitles:
              const AxisTitles(sideTitles: SideTitles(showTitles: false)),
          rightTitles:
              const AxisTitles(sideTitles: SideTitles(showTitles: false)),
        ),
        borderData: FlBorderData(show: true),
        lineBarsData: [
          LineChartBarData(
            spots: spots,
            isCurved: true,
            color: color,
            barWidth: 2,
            dotData: const FlDotData(show: false),
          ),
        ],
      ),
    );
  }

  Widget _buildRow(TelemetryReading r) {
    return Card(
      margin: const EdgeInsets.symmetric(vertical: 3),
      child: Padding(
        padding: const EdgeInsets.all(10),
        child: Row(
          children: [
            Expanded(
              child: Text(
                DateFormat('dd MMM HH:mm:ss').format(r.timestamp),
                style: const TextStyle(fontSize: 12),
              ),
            ),
            Text(r.temperature != null
                ? '${r.temperature!.toStringAsFixed(1)}°C'
                : '--'),
            const SizedBox(width: 12),
            Text(r.humidity != null
                ? '${r.humidity!.toStringAsFixed(1)}%'
                : '--'),
          ],
        ),
      ),
    );
  }
}
