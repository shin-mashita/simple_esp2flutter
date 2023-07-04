/* BLE setup adopted from https://github.com/nkolban/ESP32_BLE_Arduino/blob/master/examples/BLE_notify/BLE_notify.ino*/

#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define MPU_ENABLE
#define HC_ENABLE
#define VIB_ENABLE
// #define PLOTTER_ENABLE

#define G_MULTIPLIER 2
#define SOUND_SPEED 0.034
#define DISTANCE_THRESHOLD_CM 60

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8" 

Adafruit_MPU6050 mpu;

// int tester = 0;
const uint8_t HCtrig = 5;
const uint8_t HCecho = 18;
const uint8_t vib = 19;
const uint8_t ledpin = 2;

const float G_X = 10.39;
const float G_Y = 9.66;
const float G_Z = 12.07;

struct accgyro_t
{
    float accx;
    float accy;
    float accz;
    float gyrox;
    float gyroy;
    float gyroz;
} *accgyro;

struct acc_profile_t
{
    float buf_x[50];
    float buf_y[50];
    float buf_z[50];
    float sma_x;
    float sma_y;
    float sma_z;
} *acc_profile;

long duration;
float distance = 0;
bool fallstate = false;

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
        BLEDevice::startAdvertising();
    };

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
    }
};

/* Initial peripherals */

#if defined(MPU_ENABLE)
void init_MPU6050(void)
{
    Serial.println("Adafruit MPU6050 test!");
    // Try to initialize!
    if (!mpu.begin())
    {
        Serial.println("Failed to find MPU6050 chip");
        while (1)
        {
            delay(10);
        }
    }
    Serial.println("MPU6050 Found!");

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_260_HZ);

    accgyro = (struct accgyro_t*)calloc(sizeof(accgyro_t), sizeof(float));
    delay(100);
}
#endif

#if defined(HC_ENABLE)
void init_HC_SR04()
{
    pinMode(HCtrig, OUTPUT);
    pinMode(HCecho, INPUT);
}
#endif

#if defined(VIB_ENABLE)
void init_vib()
{
    pinMode(vib, OUTPUT);
}
#endif

/* Read sensors */

#if defined(MPU_ENABLE)
void read_accgyro()
{
    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);

    accgyro->accx = a.acceleration.x;
    accgyro->accy = a.acceleration.y;
    accgyro->accz = a.acceleration.z;

    accgyro->gyrox = g.gyro.x;
    accgyro->gyroy = g.gyro.y;
    accgyro->gyroz = g.gyro.z;
}
#endif

#if defined(HC_ENABLE)
void read_sonic()
{
    digitalWrite(HCtrig, 0);
    delayMicroseconds(2);
    digitalWrite(HCtrig, 1);
    delayMicroseconds(10);
    digitalWrite(HCtrig, 0);

    duration = pulseIn(HCecho, 1);

    distance = duration*SOUND_SPEED/2;
}
#endif

#if defined(VIB_ENABLE)
/* Motors */
void vibrateOnProximity(float threshold)
{
    if(distance < threshold)
        digitalWrite(vib, 1);
    else
        digitalWrite(vib, 0);
}
#endif

/* FALL DETECTION */

# if defined(MPU_ENABLE)
void update_acc_buf()
{
    // Dequeue structure on acc_buf
    for (size_t i = 1; i < 50; i++)
    {
        acc_profile->buf_x[i-1] = acc_profile->buf_x[i];
        acc_profile->buf_y[i-1] = acc_profile->buf_y[i];
        acc_profile->buf_z[i-1] = acc_profile->buf_z[i];
    }

    acc_profile->buf_x[49] = accgyro->accx;
    acc_profile->buf_y[49] = accgyro->accy;
    acc_profile->buf_z[49] = accgyro->accz;
}

void reinit_detect()
{
    for (size_t i = 0; i<50; i++)
    {
        read_accgyro();
        update_acc_buf();
    }
}

void init_fall_detect()
{
    acc_profile = (struct acc_profile_t*)calloc(sizeof(acc_profile_t), sizeof(float));
    Serial.println("Fall buffers initializing.");
    reinit_detect();
    Serial.println("Fall buffers initialized.");
}

void on_fall()
{
    fallstate = true;
    Serial.println("FALL DETECTED!");
    digitalWrite(ledpin, 1);
    if (deviceConnected)
    {
        if (fallstate)
            pCharacteristic->setValue("FALL");
        else
            pCharacteristic->setValue("GOOD");
        pCharacteristic->notify();
        delay(10);
    }
    delay(5000);
    fallstate = false;
    digitalWrite(ledpin, 0);
    reinit_detect();
}

void fall_detect()
{
    // Fall detection based on difference of Simple Moving Average (SMA) of last 49 samples and next sample
    update_acc_buf();

    acc_profile->sma_x = 0;
    acc_profile->sma_y = 0;
    acc_profile->sma_z = 0;
    for(size_t i = 0; i < 49; i++)acc_profile->sma_x += acc_profile->buf_x[i];
    for(size_t i = 0; i < 49; i++)acc_profile->sma_y += acc_profile->buf_y[i];
    for(size_t i = 0; i < 49; i++)acc_profile->sma_z += acc_profile->buf_z[i];

    acc_profile->sma_x /= 49;
    acc_profile->sma_y /= 49;
    acc_profile->sma_z /= 49;

    if(acc_profile->buf_x[49] - acc_profile->sma_x > G_MULTIPLIER*G_X | acc_profile->buf_y[49] - acc_profile->sma_y > G_MULTIPLIER*G_Y | acc_profile->buf_z[49] - acc_profile->sma_z > G_MULTIPLIER*G_Z) 
        on_fall();
}
#endif

/* LOGGERS */
#ifndef PLOTTER_ENABLE
#if defined(MPU_ENABLE)
void dump_all_reads()
{
    Serial.print(" ");
    Serial.print("SMA_Y: ");
    Serial.print(acc_profile->sma_y);
    Serial.print(" ");
    Serial.print("SMA_Z: ");
    Serial.print(acc_profile->sma_z);
    Serial.println("");
    Serial.print("Acceleration (m/s2): ");
    Serial.print("x = ");
    Serial.print(accgyro->accx);
    Serial.print(" ");
    Serial.print("y = ");
    Serial.print(accgyro->accy);
    Serial.print(" ");
    Serial.print("z = ");
    Serial.println(accgyro->accz);
    Serial.print("Distance (cm): ");
    Serial.println(distance);
}

void dump_acc_buf()
{
    Serial.print("X: ");
    for(size_t i = 45; i<50; i++)
    {
        Serial.print(acc_profile->buf_x[i]);
        Serial.print(" ");
    }

    Serial.print("Y: ");
    for(size_t i = 45; i<50; i++)
    {
        Serial.print(acc_profile->buf_y[i]);
        Serial.print(" ");
    }

    Serial.print("Z: ");
    for(size_t i = 45; i<50; i++)
    {
        Serial.print(acc_profile->buf_z[i]);
        Serial.print(" ");
    }

    Serial.println("");
}

void dump_acc_inits()
{
    Serial.print("Accelerometer range set to: ");
    switch (mpu.getAccelerometerRange())
    {
    case MPU6050_RANGE_2_G:
        Serial.println("+-2G");
        break;
    case MPU6050_RANGE_4_G:
        Serial.println("+-4G");
        break;
    case MPU6050_RANGE_8_G:
        Serial.println("+-8G");
        break;
    case MPU6050_RANGE_16_G:
        Serial.println("+-16G");
        break;
    }
    Serial.print("Gyro range set to: ");
    switch (mpu.getGyroRange())
    {
    case MPU6050_RANGE_250_DEG:
        Serial.println("+- 250 deg/s");
        break;
    case MPU6050_RANGE_500_DEG:
        Serial.println("+- 500 deg/s");
        break;
    case MPU6050_RANGE_1000_DEG:
        Serial.println("+- 1000 deg/s");
        break;
    case MPU6050_RANGE_2000_DEG:
        Serial.println("+- 2000 deg/s");
        break;
    }
    Serial.print("Filter bandwidth set to: ");
    switch (mpu.getFilterBandwidth())
    {
    case MPU6050_BAND_260_HZ:
        Serial.println("260 Hz");
        break;
    case MPU6050_BAND_184_HZ:
        Serial.println("184 Hz");
        break;
    case MPU6050_BAND_94_HZ:
        Serial.println("94 Hz");
        break;
    case MPU6050_BAND_44_HZ:
        Serial.println("44 Hz");
        break;
    case MPU6050_BAND_21_HZ:
        Serial.println("21 Hz");
        break;
    case MPU6050_BAND_10_HZ:
        Serial.println("10 Hz");
        break;
    case MPU6050_BAND_5_HZ:
        Serial.println("5 Hz");
        break;
    }
}
#endif

#else
void dump_for_plotter()
{
    Serial.print(accgyro->accx);
    Serial.print(" ");
    Serial.print(accgyro->accy);
    Serial.print(" ");
    Serial.print(accgyro->accz);
    Serial.print(" ");
    Serial.println(distance);
}
#endif

void setup(void)
{
    Serial.begin(115200);
    while (!Serial)
        delay(10); // will pause Zero, Leonardo, etc until serial console opens

    #if defined(MPU_ENABLE)
        init_MPU6050();
        dump_acc_inits();
        init_fall_detect();
    #endif
    
    #if defined(HC_ENABLE)
        init_HC_SR04();
    #endif

    #if defined(VIB_ENABLE)
        init_vib();
    #endif

    // Create the BLE Device
    BLEDevice::init("ESP32 SmartCrane");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_INDICATE);

    pCharacteristic->addDescriptor(new BLE2902());

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();
    Serial.println("[BLE] Waiting a client connection to notify...");
    

    pinMode(ledpin, OUTPUT);

    for(size_t i=0; i < 3; i++)
    {
        digitalWrite(ledpin, 1);
        delay(150);
        digitalWrite(ledpin, 0);
        delay(150);
    }

    Serial.println("Setup Finished.");
}

void loop()
{
    #if defined(MPU_ENABLE)
    read_accgyro();
    fall_detect();
    #endif

    #if (defined HC_ENABLE && defined VIB_ENABLE)
    read_sonic();
    vibrateOnProximity(DISTANCE_THRESHOLD_CM);
    #endif

    Serial.print("[STATUS] Fall state: ");
    Serial.print(fallstate);
    Serial.print(" | isConnected: ");
    Serial.println(deviceConnected);
    if (deviceConnected)
    {
        if (fallstate)
            pCharacteristic->setValue("FALL");
        else
            pCharacteristic->setValue("GOOD");
        pCharacteristic->notify();
        delay(10);
    }
    if (!deviceConnected && oldDeviceConnected)
    {
        delay(500);
        pServer->startAdvertising();
        Serial.println("[BLE] start advertising");
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected)
    {
        oldDeviceConnected = deviceConnected;
    }

    #if defined(MPU_ENABLE)
    dump_all_reads();
    #endif

    // if(tester%5 == 0)
    //     fallstate = true;
    // else
    //     fallstate = false;
    
    // tester++;
    delay(50);
}
