#include <WiFi.h>
#include <MFRC522.h>

#include "FirebaseESP32.h"

#define FIREBASE_HOST "https://sensor-iot-6227f.firebaseio.com/"
#define FIREBASE_AUTH "vb9IXwA7FTQuaPOEAPpICzWneKBH8rOcTjS0trVD"
#define WIFI_SSID "Yuurigami"
#define WIFI_PASSWORD "gataucuk"

#define buzzer 4
#define relay 13
#define ldr 33
#define pir 25
#define RST_PIN  22 // Reset pin
#define SS_PIN  21 // Slave select pin //MISO

FirebaseData firebaseData;
MFRC522 mfrc522(SS_PIN, RST_PIN);

String path = "/ESP32";
String pesan = "Access Ditolak";
int oldAdcLdr;
int newAdcLdr;

int stateMotion = LOW;             // default tidak ada gerakan
int valMotion = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(relay, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(ldr, INPUT);
  pinMode(pir, INPUT);

  digitalWrite(buzzer, 1);
  initWifi();

  oldAdcLdr = analogRead(ldr);
  Firebase.setInt(firebaseData, path + "/RFID", 0);
  while (!Serial); // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin(); // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial(); // Show details of PCD - MFRC522 Card Reader details
}

void systemON(){
  digitalWrite(buzzer, 0);
  delay(100);
  digitalWrite(buzzer, 1);
  delay(100);
  digitalWrite(buzzer, 0);
  delay(200);
  digitalWrite(buzzer, 1);
}

void systemOFF(){
  digitalWrite(buzzer, 0);
  delay(200);
  digitalWrite(buzzer, 1);
  delay(100);
  digitalWrite(buzzer, 0);
  delay(100);
  digitalWrite(buzzer, 1);
}

void systemDeny(){
  digitalWrite(buzzer, 0);
  delay(400);
  digitalWrite(buzzer, 1);
}

void cardReader() {
  String uid;
  String temp;

  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  for (int i = 0; i < 4; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      temp = "0" + String(mfrc522.uid.uidByte[i], HEX);
    } else {
      temp = String(mfrc522.uid.uidByte[i], HEX);
    }
    if (i == 3) {
      uid =  uid + temp;
    } else {
      uid =  uid + temp + " ";
    }
  }
  Serial.println("UID " + uid);
  String grantedAccess = "d9 f3 09 c3";
  grantedAccess.toLowerCase();

  if (uid == grantedAccess) {
    if (pesan == "Access Ditolak") {
      Serial.println("Access Diterima");
      Firebase.setInt(firebaseData, path + "/RFID", 1);
      systemON();
      pesan = "Access Diterima";
    } else {
      Serial.println("System Off");
      Firebase.setInt(firebaseData, path + "/RFID", 0);
      systemOFF();
      pesan = "Access Ditolak";
    }
  } else {
    Serial.println("Access Ditolak");
    systemDeny();
  }
  Serial.println("\n");
  mfrc522.PICC_HaltA();
}


void loop() {
  // put your main code here, to run repeatedly:
  delay(500);
  cardReader();
  if (pesan == "Access Diterima") {
    
    valMotion = digitalRead(pir);   // read sensor value
    if (valMotion == HIGH) {           // check if the sensor is HIGH
      if (stateMotion == LOW) {
        Firebase.setInt(firebaseData, path + "/pir", 1);
        stateMotion = HIGH;       // update variable state to HIGH
      }
    }
    else {
      if (stateMotion == HIGH) {
        Firebase.setInt(firebaseData, path + "/pir", 0);
        stateMotion = LOW;       // update variable state to LOW
      }
    }


    //ambil nilai dari sensor LDR dan kirim ke firebase
    newAdcLdr = analogRead(ldr);
    //Serial.println(newAdcLdr);
    if (newAdcLdr != oldAdcLdr) {
      Firebase.setDouble(firebaseData, path + "/cahaya", newAdcLdr);
      oldAdcLdr = newAdcLdr;
    }

    //get value
    if (Firebase.getInt(firebaseData, path + "/relay")) {
      /*Serial.println("PASSED");
        Serial.println("PATH: " + firebaseData.dataPath());
        Serial.println("TYPE: " + firebaseData.dataType());
        Serial.println("ETag: " + firebaseData.ETag());
        Serial.print("VALUE: ");
        Serial.println(firebaseData.intData());
      */
      int condition;
      if (digitalRead(pir) == 1 && analogRead(ldr) >= 3000) {
        condition = 1;
      } else {
        condition = 0;
      }

      if (condition == 0 && firebaseData.intData() == 0)
        digitalWrite(relay, 0);
      else
        digitalWrite(relay, 1);
    }

  }

}

void initWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  //Set database read timeout to 1 minute (max 15 minutes)
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  //tiny, small, medium, large and unlimited.
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.setwriteSizeLimit(firebaseData, "tiny");
}
