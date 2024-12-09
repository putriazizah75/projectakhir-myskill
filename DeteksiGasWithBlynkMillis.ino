#define BLYNK_TEMPLATE_ID "TMPL6hCsy4f14"
#define BLYNK_TEMPLATE_NAME "Smoke Detector Final Project Myskill"
#define BLYNK_AUTH_TOKEN "k8_PEUXsSAiaPy4pTm4ZB_HzmmTOkqkW"

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// Informasi WiFi
const char* ssid = "Anita Taufik";          // Ganti dengan nama WiFi
const char* pass = "talanggading";      // Ganti dengan password WiFi

// Pin konfigurasi
const int pinMQ135 = 34;     // Pin analog untuk MQ135
const int pinRed = 25;       // Pin untuk LED merah (RGB)
const int pinGreen = 26;     // Pin untuk LED hijau (RGB)
const int pinBlue = 27;      // Pin untuk LED biru (RGB)
const int pinBuzzer = 32;    // Pin untuk buzzer
const int pinRelay = 33;     // Pin untuk kontrol relay (kipas)

// Variabel kalibrasi
float RLOAD = 10.0;          // Nilai resistor load
float RZERO = 76.63;         // Nilai RZERO sesuai kalibrasi (disesuaikan)
float CO2Curve[3] = {2.602, 0.2, -0.48}; // Kurva kalibrasi CO2 (berdasarkan datasheet)

// Variabel untuk waktu dengan millis()
unsigned long previousMillisBuzzer = 0;
unsigned long buzzerInterval = 200; // Interval waktu untuk buzzer
unsigned long previousMillisSensor = 0;
unsigned long sensorInterval = 10000; // Interval waktu untuk pembacaan sensor
bool buzzerState = false;

// Variabel kontrol manual
bool controlfan = false; // Status kontrol manual relay

// Widget LED di Blynk
WidgetLED ledRedWidget(V1);
WidgetLED ledYellowWidget(V2);
WidgetLED ledGreenWidget(V3);
WidgetLED buzzerWidget(V4);
WidgetLED kipasWidget(V5);

void setup() {
  Serial.begin(115200);

  // Koneksi WiFi dan Blynk
  WiFi.begin(ssid, pass);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Konfigurasi pin mode
  pinMode(pinRed, OUTPUT);
  pinMode(pinGreen, OUTPUT);
  pinMode(pinBlue, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);
  pinMode(pinRelay, OUTPUT);
  // digitalWrite(pinRelay, LOW);
  delay(2000);

  // Preheat sensor MQ-135 dengan hitungan mundur
  Serial.println("Preheating MQ-135 sensor...");
  for (int i = 10; i > 0; i--) {
    Serial.print("Waktu preheat: ");
    Serial.print(i);
    Serial.println(" detik");
    delay(1000); // Jeda 1 detik
  }
  Serial.println("Preheat selesai. Inisialisasi CO2 Detector");
}

void loop() {
  Blynk.run();

  unsigned long currentMillis = millis();

  // Bacaan sensor dilakukan sesuai interval
  static float ppmCO2 = 0; // Menyimpan nilai ppm terakhir
  if (currentMillis - previousMillisSensor >= sensorInterval) {
    previousMillisSensor = currentMillis;
    int sensorValue = analogRead(pinMQ135);
    ppmCO2 = calculatePPM(sensorValue);

    // Kirim data ke Blynk
    Blynk.virtualWrite(V0, ppmCO2); // Kirim data ppm CO2 ke virtual pin V0

    Serial.print("Kadar CO2 (ppm): ");
    Serial.println(ppmCO2);

    // Indikasi LED dan buzzer berdasarkan kadar CO2

    // 1. KONDISI NORMAL (AMAN)
    if (ppmCO2 <= 750) {
      setRGBColor(0, 255, 0);      // Hijau (RGB)
      ledRedWidget.off();          // Matikan widget LED merah
      ledYellowWidget.off();       // Matikan widget LED kuning
      ledGreenWidget.on();         // Nyalakan widget LED hijau
      noTone(pinBuzzer);           // Buzzer mati
      buzzerWidget.off();         // Widget buzzer mati
      kipasWidget.off();         // Widget kipas mati
      digitalWrite(pinRelay, HIGH); // Kipas mati (relay tidak aktif)
      Blynk.virtualWrite(V5, LOW);

    // 2. KONDISI SEDANG (WASPADA)
    } else if (ppmCO2 > 750 && ppmCO2 <= 1200) {
      setRGBColor(255, 255, 0);    // Kuning (RGB)
      ledRedWidget.off();          // Matikan widget LED merah
      ledYellowWidget.on();        // Nyalakan widget LED kuning
      ledGreenWidget.off();        // Matikan widget LED hijau
      tone(pinBuzzer, 1000);       // Buzzer dengan frekuensi 1000 Hz
      delay(200);                   
      noTone(pinBuzzer);           
      delay(200);      
      buzzerWidget.on();         // Widget buzzer hidup
      kipasWidget.on();         // Widget kipas hidup             
      digitalWrite(pinRelay, LOW); // Kipas menyala (relay aktif)
      Blynk.virtualWrite(V5, HIGH);
      Blynk.logEvent("Gas_Notification", "Kadar Gas CO2 Terdeteksi Cukup Tinggi!! Waspadalah!!"); // Menampilkan Notifikasi

    // 3. KONDISI TINGGI (BAHAYA)
    } else {
      setRGBColor(255, 0, 0);      // Merah (RGB)
      ledRedWidget.on();           // Nyalakan widget LED merah
      ledYellowWidget.off();       // Matikan widget LED kuning
      ledGreenWidget.off();        // Matikan widget LED hijau
      tone(pinBuzzer, 2000);       // Buzzer dengan frekuensi 2000 Hz
      delay(100);                  
      noTone(pinBuzzer);           
      delay(100);           
      buzzerWidget.on();         // Widget buzzer hidup
      kipasWidget.on();         // Widget kipas hidup         
      digitalWrite(pinRelay, LOW); // Kipas menyala (relay aktif)
      Blynk.virtualWrite(V5, HIGH);
      Blynk.logEvent("Gas_Notification", "Kadar Gas CO2 Terdeteksi Sangat Tinggi!! Segera Periksa Ruangan Sekarang!!"); // Menampilkan Notifikasi
    }
  }

  // Kontrol buzzer menggunakan millis()
  if (ppmCO2 > 750) { // Aktifkan buzzer jika ppm di atas 750
    if (currentMillis - previousMillisBuzzer >= buzzerInterval) {
      previousMillisBuzzer = currentMillis;
      buzzerState = !buzzerState; // Toggle state buzzer
      int frequency = (ppmCO2 <= 1200) ? 1000 : 2000; // Tentukan frekuensi (1000 Hz untuk sedang, 2000 Hz untuk bahaya)
      if (buzzerState) {
        tone(pinBuzzer, frequency);
      } else {
        noTone(pinBuzzer);
      }
    }
  } else {
    noTone(pinBuzzer); // Buzzer mati jika kondisi aman
  }
}

// Fungsi untuk mengatur warna LED RGB fisik
void setRGBColor(int red, int green, int blue) {
  analogWrite(pinRed, red);
  analogWrite(pinGreen, green);
  analogWrite(pinBlue, blue);
}

// Fungsi untuk menghitung ppm CO2 berdasarkan nilai sensor
float calculatePPM(int sensorValue) {
  float VRL = (sensorValue / 4095.0) * 5.0;  // Menghitung tegangan output
  float RS = (5.0 - VRL) / VRL * RLOAD;      // Menghitung resistansi RS
  float ratio = RS / RZERO;                    // Rasio resistansi RS dengan RZERO
  float ppm = pow(10, ((log10(ratio) - CO2Curve[1]) / CO2Curve[2]) + CO2Curve[0]); // Perhitungan ppm CO2
  return ppm;
}

// Function control relay manual
BLYNK_WRITE(V6) {
  controlfan = param.asInt();
  if (controlfan) {
    digitalWrite(pinRelay, LOW); // Kipas menyala (relay aktif)
    Blynk.virtualWrite(V5, HIGH);
  } else {
    digitalWrite(pinRelay, HIGH); // Kipas mati (relay tidak aktif)
    Blynk.virtualWrite(V5, LOW);
  }
}
