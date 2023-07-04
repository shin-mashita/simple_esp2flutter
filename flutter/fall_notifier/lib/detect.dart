import 'package:flutter/material.dart';
import 'package:flutter_local_notifications/flutter_local_notifications.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'dart:developer';
import 'dart:async';
import 'dart:io';

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key});

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  static final FlutterBluePlus flutterBlue = FlutterBluePlus.instance;
  static bool readState = false;
  static int idx = 0;
  var prompts = [
    "Welcome",
    "Disconnected to SmartCane",
    "SmartCane not found",
    "Connected to SmartCane",
    "Detecting falls...",
    "FALL DETECTED!!!",
    "Notifier on standby",
    "Turn on Bluetooth",
    "Still reading."
  ];

  void displayPrompt(int val) {
    setState(() {
      idx = val;
    });
    log("Changing prompt.");
  }

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) => bleDisconnectAll());
    WidgetsBinding.instance.addPostFrameCallback((_) => displayPrompt(0));
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('SmartCane Notifier'),
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          crossAxisAlignment: CrossAxisAlignment.center,
          children: [
            Image.asset(
              'assets/images/app_icon.png',
              height: 100,
              width: 100,
            ),
            Text(
              "SmartCane Notifier",
              textAlign: TextAlign.center,
              style: const TextStyle(fontSize: 25),
            ),
            const SizedBox(height: 85),
            Text(
              prompts[idx],
              textAlign: TextAlign.center,
              style: const TextStyle(fontSize: 20),
            ),
            const SizedBox(height: 75),
            ElevatedButton(
              style: ElevatedButton.styleFrom(
                  padding: const EdgeInsets.fromLTRB(32.5, 10, 32.5, 10)),
              child: const Text('Connect', style: TextStyle(fontSize: 20)),
              onPressed: () async {
                bool bleState = await flutterBlue.isOn;
                log("BLE state: $bleState");
                if (bleState) {
                  if(!readState)
                  {
                    bleScan();
                  }
                  else {
                    displayPrompt(8);
                  }
                } else {
                  displayPrompt(7);
                }
              },
            ),
            const SizedBox(height: 15),
            ElevatedButton(
              style: ElevatedButton.styleFrom(
                  padding: const EdgeInsets.fromLTRB(20, 10, 20, 10)),
              child: const Text('Disconnect', style: TextStyle(fontSize: 20)),
              onPressed: () {
                if(!readState){
                  bleDisconnectAll();
                }
                else{
                  displayPrompt(8);
                }
                
              },
            ),
            const SizedBox(height: 15),
            ElevatedButton(
              style: ElevatedButton.styleFrom(
                  padding: const EdgeInsets.fromLTRB(22.5, 10, 22.5, 10)),
              child: const Text('Detect Fall', style: TextStyle(fontSize: 20)),
              onPressed: () {
                readState = !readState;
                log("Readstate: $readState");

                if (readState) {
                  displayPrompt(4);
                  BluetoothDevice device =
                      BluetoothDevice.fromId('78:E3:6D:18:DF:0E');
                  bleContinuousRead(device);
                }
              },
            )
          ],
        ),
      ),
    );
  }

  bleReadCharacteristics(BluetoothDevice device) async {
    List<BluetoothService> services = await device.discoverServices();

    for (BluetoothService service in services) {
      var characteristics = service.characteristics;
      for (BluetoothCharacteristic c in characteristics) {
        if (c.properties.read &&
            c.uuid == Guid("beb5483e-36e1-4688-b7f5-ea07361b26a8")) {
          List<int> value = await c.read();
          String signal = String.fromCharCodes(value);
          log(signal);

          if (signal == "FALL") {
            displayPrompt(5);
            showNotification();
            await Future.delayed(const Duration(seconds: 5));
            displayPrompt(4);
          }
        }
      }
    }
  }

  bleContinuousRead(BluetoothDevice device) async {
    while (true) {
      if (!readState) {
        log("Readstate: $readState");
        log("Ending read.");
        displayPrompt(6);
        break;
      } else {
        await bleReadCharacteristics(device);
      }
    }
  }

  bleConnect(BluetoothDevice device) async {
    List<BluetoothDevice> connectedDevices = await flutterBlue.connectedDevices;
    if (!connectedDevices.contains(device)) {
      log('Connecting to ${device.name}');
      await device.connect(timeout: const Duration(seconds: 4));
    }

    connectedDevices = await flutterBlue.connectedDevices;
    if (connectedDevices.contains(device)) {
      log('Connected a successfully');
      displayPrompt(3);
    }

    // BLE_read_characteristics(device);
    flutterBlue.stopScan();
  }

  bleDisconnectAll() async {
    List<BluetoothDevice> connectedDevices = await flutterBlue.connectedDevices;
    for (BluetoothDevice device in connectedDevices) {
      log('Disconnecting: ${device.name}');
      await device.disconnect();
    }
    displayPrompt(1);
  }

  bleScan() async {
    BluetoothDevice device;

    FlutterBluePlus.instance
        .startScan(timeout: const Duration(milliseconds: 100));

    FlutterBluePlus.instance.stopScan();

    Timer(const Duration(milliseconds: 100), () {
      flutterBlue.startScan(timeout: const Duration(seconds: 1));
    });

    flutterBlue.scanResults.listen((results) {
      for (ScanResult r in results) {
        log('${r.device.name} found! rssi: ${r.rssi}');

        if (r.device.id == const DeviceIdentifier('78:E3:6D:18:DF:0E')) {
          device = r.device;
          bleConnect(device);
        }
      }
      displayPrompt(2);
    });

    if (flutterBlue.isScanningNow) {
      flutterBlue.stopScan();
      log('done scanning');
    }
  }

  showNotification() async {
    FlutterLocalNotificationsPlugin flutterLocalNotificationsPlugin =
        FlutterLocalNotificationsPlugin();

    const AndroidInitializationSettings initializationSettingsAndroid =
        AndroidInitializationSettings('@mipmap/ic_launcher');

    const IOSInitializationSettings initializationSettingsIOS =
        IOSInitializationSettings(
      requestSoundPermission: false,
      requestBadgePermission: false,
      requestAlertPermission: false,
    );
    const InitializationSettings initializationSettings =
        InitializationSettings(
      android: initializationSettingsAndroid,
      iOS: initializationSettingsIOS,
    );

    await flutterLocalNotificationsPlugin.initialize(
      initializationSettings,
    );

    AndroidNotificationChannel channel = const AndroidNotificationChannel(
      'high channel',
      'Very important notification!!',
      description: 'the first notification',
      importance: Importance.max,
    );

    await flutterLocalNotificationsPlugin.show(
      1,
      'Fall detected!',
      'A fall is detected from the SmartCane user.',
      NotificationDetails(
        android: AndroidNotificationDetails(channel.id, channel.name,
            channelDescription: channel.description),
      ),
    );
  }
}
