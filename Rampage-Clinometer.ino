// Talon Finehout, Matthew Bishop
#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <Wire.h>
#include <math.h>

// -------------------- MPU --------------------
const int MPU = 0x68;
long MPUtime;

// -------------------- Ultrasonic --------------------
#define ULTRA_TRIG 7
#define ULTRA_ECHO 6

// -------------------- Struct --------------------
typedef struct {
  int pitch;
  int roll;
  int distance;
} SensorData_t;

// -------------------- Queue --------------------
QueueHandle_t sensorQueue;

// -------------------- Accelerometer --------------------
float accX, accY, accZ;
int sumAX, sumAY, sumAZ;
float avgAX, avgAY, avgAZ;
float convert = 16384.0;

bool calibrate = false;

// -------------------- Task Prototypes --------------------
void TaskGetData(void *pvParameters);
void TaskSendData(void *pvParameters);

// -------------------- Setup --------------------
void setup() {

  Serial.begin(9600);
  while (!Serial) {}

  pinMode(ULTRA_TRIG, OUTPUT);
  pinMode(ULTRA_ECHO, INPUT);

  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  // Create queue for struct
  sensorQueue = xQueueCreate(5, sizeof(SensorData_t));
  if (sensorQueue == NULL) {
    Serial.println("Queue creation failed!");
    while (1);
  }

  // Create tasks
  xTaskCreate(TaskGetData,  "GetData",  256, NULL, 2, NULL);
  xTaskCreate(TaskSendData, "SendData", 256, NULL, 1, NULL);
}

// -------------------- Loop --------------------
void loop() {
  // Not used with FreeRTOS
}

/*--------------------------------------------------*/
/*-------------------- TASKS ------------------------*/
/*--------------------------------------------------*/

void TaskGetData(void *pvParameters) {
  (void) pvParameters;

  SensorData_t data;

  for (;;) {

    // ---- Ultrasonic ----
    digitalWrite(ULTRA_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(ULTRA_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRA_TRIG, LOW);

    MPUtime = pulseIn(ULTRA_ECHO, HIGH, 30000);
    if (MPUtime > 0) {
      data.distance = MPUtime * 0.0343 / 2.0;
    } else {
      data.distance = -1;
    }

    // ---- Accelerometer calibration ----
    if (!calibrate) {
      sumAX = sumAY = sumAZ = 0;

      for (int i = 0; i < 100; i++) {
        Wire.beginTransmission(MPU);
        Wire.write(0x3B);
        Wire.endTransmission(false);
        Wire.requestFrom(MPU, 6, true);

        sumAX += (Wire.read() << 8 | Wire.read());
        sumAY += (Wire.read() << 8 | Wire.read());
        sumAZ += (Wire.read() << 8 | Wire.read());

        vTaskDelay(100 / portTICK_PERIOD_MS);
      }

      avgAX = (sumAX / 100.0) / convert;
      avgAY = (sumAY / 100.0) / convert;
      avgAZ = (sumAZ / 100.0) / convert;
      calibrate = true;
    }

    // ---- Read accelerometer ----
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 6, true);

    accX = (Wire.read() << 8 | Wire.read()) / convert - avgAX;
    accY = (Wire.read() << 8 | Wire.read()) / convert - avgAY;
    accZ = (Wire.read() << 8 | Wire.read()) / convert - avgAZ;

    // ---- Convert to degrees ----
    float roll  = atan2(accY, accZ) * 180.0 / PI;
    float pitch = atan2(-accX, sqrt(accY * accY + accZ * accZ)) * 180.0 / PI;

    data.roll  = round(roll);
    data.pitch = round(pitch);

    // ---- Send struct ----
    xQueueSend(sensorQueue, &data, portMAX_DELAY);

    vTaskDelay(50 / portTICK_PERIOD_MS); // 5 Hz
  }
}

void TaskSendData(void *pvParameters) {
  (void) pvParameters;

  SensorData_t rx;

  for (;;) {
    if (xQueueReceive(sensorQueue, &rx, portMAX_DELAY) == pdPASS) {

      vTaskDelay(25 / portTICK_PERIOD_MS); // 2.5 Hz

      Serial.print(rx.roll);
      Serial.print(",");
      Serial.print(rx.pitch);
      Serial.print(",");
      Serial.println(rx.distance);
    }
  }
}
