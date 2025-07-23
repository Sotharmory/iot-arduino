#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

Servo servo_1;
const int servoPin = 3; 

void setup() {
  Serial.begin(9600); 
  SPI.begin();
  mfrc522.PCD_Init();
  servo_1.attach(servoPin);
  servo_1.write(0); // Initial position
}

void loop() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
      uid += String(mfrc522.uid.uidByte[i], HEX);
      if (i < mfrc522.uid.size - 1) uid += ":";
    }
    Serial.print("NFC_UID:");
    Serial.println(uid);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    delay(500); // debounce
  }

  // Handle commands from ESP32
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.startsWith("WRITE_NFC:")) {
      String payload = cmd.substring(10);
      byte block = 4;
      MFRC522::MIFARE_Key key;
      for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
      if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
          MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid)
        );
        if (status == MFRC522::STATUS_OK) {
          byte buffer[16];
          for (byte i = 0; i < 16; i++) {
            if (i < payload.length()) buffer[i] = payload[i];
            else buffer[i] = 0x00;
          }
          status = mfrc522.MIFARE_Write(block, buffer, 16);
          if (status == MFRC522::STATUS_OK) {
            Serial.println("NFC_WRITE_OK");
          } else {
            Serial.println("NFC_WRITE_FAIL");
          }
        } else {
          Serial.println("NFC_WRITE_FAIL");
        }
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
      }
    } 
    // Move Servo
    else if (cmd.startsWith("SERVO:")) {
      // Move from 0 to 180
      for (int pos = 0; pos <= 180; pos++) {
        servo_1.write(pos);
        delay(15);
        if (pos == 90) delay(5000); // Pause at middle
      }

      delay(5000); // Wait after reaching 180

      // Move back from 180 to 0
      for (int pos = 180; pos >= 0; pos--) {
        servo_1.write(pos);
        delay(15);
        if (pos == 90) delay(5000); // Pause at middle
      }

      delay(5000); // Wait at end
      Serial.println("SERVO_OK");
    }

  }
}
