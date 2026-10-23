import 'package:flutter/material.dart';
import 'package:intl/intl.dart';
import '../models/models.dart';
import '../services/api_service.dart';

class AlertsScreen extends StatefulWidget {
  final ApiService api;
  final String deviceId;

  const AlertsScreen({super.key, required this.api, required this.deviceId});

  @override
  State<AlertsScreen> createState() => _AlertsScreenState();
}

class _AlertsScreenState extends State<AlertsScreen>
    with SingleTickerProviderStateMixin {
  late TabController _tabs;
  List<Alert> _active = [];
  List<Alert> _all    = [];
  bool _loading = true;
  String? _error;

  @override
  void initState() {
    super.initState();
    _tabs = TabController(length: 2, vsync: this);
    _load();
  }

  @override
  void dispose() {
    _tabs.dispose();
    super.dispose();
  }

  Future<void> _load() async {
    setState(() {
      _loading = true;
      _error = null;
    });
    try {
      final active = await widget.api.getActiveAlerts(widget.deviceId);
      final all    = await widget.api.getAlerts(widget.deviceId);
      setState(() {
        _active  = active.items;
        _all     = all.items;
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
        title: const Text('Alerts'),
        actions: [
          IconButton(icon: const Icon(Icons.refresh), onPressed: _load)
        ],
        bottom: TabBar(
          controller: _tabs,
          tabs: [
            Tab(text: 'Active (${_active.length})'),
            Tab(text: 'All (${_all.length})'),
          ],
        ),
      ),
      body: _loading
          ? const Center(child: CircularProgressIndicator())
          : _error != null
              ? Center(
                  child: Column(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Text(_error!, textAlign: TextAlign.center),
                      const SizedBox(height: 12),
                      ElevatedButton(
                          onPressed: _load, child: const Text('Retry')),
                    ],
                  ),
                )
              : TabBarView(
                  controller: _tabs,
                  children: [
                    _buildList(_active, showResolve: true),
                    _buildList(_all,    showResolve: false),
                  ],
                ),
    );
  }

  Widget _buildList(List<Alert> alerts, {required bool showResolve}) {
    if (alerts.isEmpty) {
      return const Center(child: Text('No alerts.'));
    }
    return ListView.builder(
      padding: const EdgeInsets.all(12),
      itemCount: alerts.length,
      itemBuilder: (ctx, i) => _AlertTile(
        alert: alerts[i],
        showResolve: showResolve && alerts[i].isActive,
        onResolve: () async {
          await widget.api.resolveAlert(alerts[i].id);
          _load();
        },
      ),
    );
  }
}

class _AlertTile extends StatelessWidget {
  final Alert alert;
  final bool showResolve;
  final VoidCallback? onResolve;

  const _AlertTile(
      {required this.alert, required this.showResolve, this.onResolve});

  Color get _color => alert.isActive ? Colors.red : Colors.grey;

  String get _label => alert.alertType.replaceAll('_', ' ');

  @override
  Widget build(BuildContext context) {
    return Card(
      margin: const EdgeInsets.symmetric(vertical: 5),
      child: ListTile(
        leading: Icon(
          alert.isActive ? Icons.warning_amber : Icons.check_circle_outline,
          color: _color,
        ),
        title: Text(_label,
            style: TextStyle(fontWeight: FontWeight.bold, color: _color)),
        subtitle: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Value: ${alert.value.toStringAsFixed(1)} '
                '(threshold: ${alert.threshold.toStringAsFixed(1)})'),
            Text(DateFormat('dd MMM yyyy HH:mm').format(alert.timestamp),
                style: const TextStyle(fontSize: 11, color: Colors.grey)),
            if (!alert.isActive && alert.resolvedAt != null)
              Text(
                  'Resolved: ${DateFormat('dd MMM HH:mm').format(alert.resolvedAt!)}',
                  style: const TextStyle(fontSize: 11, color: Colors.green)),
          ],
        ),
        trailing: showResolve
            ? TextButton(
                onPressed: onResolve,
                child: const Text('Resolve'),
              )
            : null,
      ),
    );
  }
}
