import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'services/api_service.dart';
import 'screens/dashboard_screen.dart';
import 'screens/history_screen.dart';
import 'screens/alerts_screen.dart';
import 'screens/settings_screen.dart';

const String kDefaultDeviceId  = 'agrihub-001';
const String kDefaultBackendUrl = 'http://10.0.2.2:8000'; // Android emulator → localhost

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  final prefs = await SharedPreferences.getInstance();
  final url   = prefs.getString('backend_url') ?? kDefaultBackendUrl;
  runApp(AgriCoolHubApp(initialUrl: url));
}

class AgriCoolHubApp extends StatefulWidget {
  final String initialUrl;
  const AgriCoolHubApp({super.key, required this.initialUrl});

  @override
  State<AgriCoolHubApp> createState() => _AgriCoolHubAppState();
}

class _AgriCoolHubAppState extends State<AgriCoolHubApp> {
  late String _backendUrl;

  @override
  void initState() {
    super.initState();
    _backendUrl = widget.initialUrl;
  }

  void _onUrlChanged(String url) {
    setState(() => _backendUrl = url);
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Agri Cool Hub',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.green),
        useMaterial3: true,
      ),
      home: _HomeShell(
        backendUrl:   _backendUrl,
        onUrlChanged: _onUrlChanged,
      ),
    );
  }
}

class _HomeShell extends StatefulWidget {
  final String backendUrl;
  final ValueChanged<String> onUrlChanged;

  const _HomeShell({required this.backendUrl, required this.onUrlChanged});

  @override
  State<_HomeShell> createState() => _HomeShellState();
}

class _HomeShellState extends State<_HomeShell> {
  int _selectedIndex = 0;

  @override
  Widget build(BuildContext context) {
    final api = ApiService(widget.backendUrl);

    final screens = [
      DashboardScreen(api: api, deviceId: kDefaultDeviceId),
      HistoryScreen(api: api,   deviceId: kDefaultDeviceId),
      AlertsScreen(api: api,    deviceId: kDefaultDeviceId),
      SettingsScreen(
        currentUrl:   widget.backendUrl,
        onUrlChanged: widget.onUrlChanged,
      ),
    ];

    return Scaffold(
      body: IndexedStack(index: _selectedIndex, children: screens),
      bottomNavigationBar: NavigationBar(
        selectedIndex: _selectedIndex,
        onDestinationSelected: (i) => setState(() => _selectedIndex = i),
        destinations: const [
          NavigationDestination(icon: Icon(Icons.dashboard),  label: 'Dashboard'),
          NavigationDestination(icon: Icon(Icons.show_chart), label: 'History'),
          NavigationDestination(icon: Icon(Icons.notifications), label: 'Alerts'),
          NavigationDestination(icon: Icon(Icons.settings),   label: 'Settings'),
        ],
      ),
    );
  }
}
