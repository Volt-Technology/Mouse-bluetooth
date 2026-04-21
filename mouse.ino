#include <Wire.h>
#include <MPU6050.h>
#include <BleMouse.h>

MPU6050 sensor;
BleMouse bleMouse("Mouse Bluetooth Ropiul");

int16_t ax, ay, az;
int16_t gx, gy, gz;

float sensitivitas    = 0.0033; // mengatur sensitivitas
float kehalusanGerak  = 0.1; // kehalusan gerak
float kecepatanScroll = 10.0; // kecepatan scroll
int threshold = 2000;
const int waktuKalibrasi = 3000; // kalibrasi sensor

//Konfig Pin
const int led = 2; 
const int klikKiri = 25; 
const int klikKanan = 26;
const int scroll = 27;

//Inisisalisasi
unsigned long mulai = 0;
unsigned long akhir = 0;
float xhalus = 0.0;
float yhalus = 0.0;
int calGX = 0;
int calGY = 0;
uint8_t diam = false;
uint8_t scrollMode = false;
uint8_t ledState = false;

bool miring(){
  sensor.getAcceleration(&ax, &ay, &az);
  return(abs(ax) > abs(az) * 1.5);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  pinMode(led, OUTPUT);
  pinMode(klikKiri, INPUT_PULLUP);
  pinMode(klikKanan, INPUT_PULLUP);
  pinMode(scroll, INPUT_PULLUP);

  sensor.initialize();
  sensor.setFullScaleGyroRange(MPU6050_GYRO_FS_250);
  sensor.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
  sensor.setDLPFMode(MPU6050_DLPF_BW_20);

  if(!sensor.testConnection()){
    Serial.println("Sensor Tidak Terhubung");
    while(true);
  }
  
  Serial.println("Kalibrasi...");
  kalibrasi();
  bleMouse.begin();
}

void loop() {
  if(!bleMouse.isConnected()){
    if(millis() - akhir >= 100){
      Serial.println("Mouse Belum Terhubung");
      akhir = millis();
      ledState = !ledState;
      digitalWrite(led, ledState);
    }
    return;
  }else digitalWrite(led, HIGH);

  sensor.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  gx -= calGX;
  gy -= calGY;

  bool gerakKecil = (abs(gx) < threshold && abs(gy) < threshold);

  if(gerakKecil){
    if(!diam){
      mulai = millis();
      diam = true;
    }
    
    if(millis() - mulai >= waktuKalibrasi){
      kalibrasi();
      mulai = millis();
      diam = true;
    }else diam = false;
  }

  if(abs(gx) < threshold) gx = 0;
  if(abs(gy) < threshold) gy = 0;

  float rawX;
  float rawY;

  if(miring()){
    rawX = gx * sensitivitas;
    rawY = gz * sensitivitas;
  }else{
    rawX = gy * sensitivitas;
    rawY = -gx * sensitivitas;
  }

  xhalus = xhalus + kehalusanGerak * (rawX - xhalus);
  yhalus = yhalus + kehalusanGerak * (rawY- yhalus);

  if(gx == 0) xhalus = xhalus * 0.8;
  if(gy == 0) yhalus = yhalus * 0.8;

  int perpindahanX = (int)xhalus;
  int perpindahanY = (int)yhalus;

  //Mode Scroll
  scrollMode = (digitalRead(scroll) == LOW);
  if(scrollMode){
    if(abs(perpindahanY) > 1) bleMouse.move(0, 0, (int)perpindahanY / kecepatanScroll);
  }else {
    if(perpindahanX != 0 || perpindahanY != 0){
      bleMouse.move(perpindahanX, perpindahanY);
    }
  }

  //Tombol Kiri
  if(digitalRead(klikKiri) == LOW){
    bleMouse.click(MOUSE_LEFT);
    delay(150);
  }

  //TOmbol Kanan
  if(digitalRead(klikKanan) == LOW){
    bleMouse.click(MOUSE_RIGHT);
    delay(300);
  }

  delay(5);
}

void kalibrasi(){
  long sumGX = 0, sumGY = 0;
  int sampel = 200;

  //Test 200 kali
  for(int i = 0; i < sampel; i++){
    sensor.getRotation(&gx, &gy, &gz);
    sumGX += gx;
    sumGY += gy;
    delay(5);
  }

  calGX = sumGX/sampel;
  calGY = sumGY/sampel;
}
