#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "pitches.h"

// ===================== KONFIGURACJA =====================

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

#define API_KEY ""
#define DATABASE_URL ""
#define USER_EMAIL ""
#define USER_PASSWORD ""

const int buzzerPin = 18;
const int tempPin = 23;

OneWire oneWire(tempPin);
DallasTemperature tempSensor(&oneWire);

const int song0Notes[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3,
  NOTE_G3, REST, NOTE_B3, NOTE_C4
};
const int song0Durations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

const int song1Notes[] = {
  NOTE_C4, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_F4, NOTE_E4,
  NOTE_C4, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_G4, NOTE_F4,
  NOTE_C4, NOTE_C4, NOTE_C5, NOTE_A4, NOTE_F4, NOTE_E4, NOTE_D4,
  NOTE_AS4, NOTE_AS4, NOTE_A4, NOTE_F4, NOTE_G4, NOTE_F4
};
const int song1Durations[] = {
  8, 8, 4, 4, 4, 2,
  8, 8, 4, 4, 4, 2,
  8, 8, 4, 4, 4, 4, 2,
  8, 8, 4, 4, 4, 2
};

const int song2Notes[] = {
  NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4,
  NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4,
  NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4,
  NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4,
  NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4
};
const int song2Durations[] = {
  4, 4, 4, 4, 4, 4, 2,
  4, 4, 4, 4, 4, 4, 2,
  4, 4, 4, 4, 4, 4, 2,
  4, 4, 4, 4, 4, 4, 2,
  4, 4, 4, 4, 4, 4, 2
};

const int song3Notes[] = {
  NOTE_E5, NOTE_E5, REST, NOTE_E5, REST, NOTE_C5, NOTE_E5, REST,
  NOTE_G5, REST, REST, NOTE_G4
};
const int song3Durations[] = {
  8, 8, 8, 8, 8, 8, 8, 8,
  4, 8, 4, 4
};

const int song4Notes[] = {
  NOTE_E4, NOTE_E4, NOTE_E4,
  NOTE_E4, NOTE_E4, NOTE_E4,
  NOTE_E4, NOTE_G4, NOTE_C4, NOTE_D4, NOTE_E4,
  NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_E4, NOTE_E4,
  NOTE_D4, NOTE_D4, NOTE_E4, NOTE_D4, NOTE_G4
};
const int song4Durations[] = {
  8, 8, 4,
  8, 8, 4,
  8, 8, 8, 8, 2,
  8, 8, 8, 8, 8, 8, 8, 16, 16,
  8, 8, 8, 4, 4
};

struct Song {
  const char* name;
  const int* notes;
  const int* durations;
  int length;
};

const Song songs[] = {
  { "Prosta melodia", song0Notes, song0Durations, sizeof(song0Notes) / sizeof(song0Notes[0]) },
  { "Sto lat", song1Notes, song1Durations, sizeof(song1Notes) / sizeof(song1Notes[0]) },
  { "Gwiazdeczka", song2Notes, song2Durations, sizeof(song2Notes) / sizeof(song2Notes[0]) },
  { "Mario", song3Notes, song3Durations, sizeof(song3Notes) / sizeof(song3Notes[0]) },
  { "Jingle Bells", song4Notes, song4Durations, sizeof(song4Notes) / sizeof(song4Notes[0]) }
};

const int songCount = sizeof(songs) / sizeof(songs[0]);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

const char* playPath = "/board1/buzzer/play";
const char* songPath = "/board1/buzzer/song";
const char* tempPath = "/board1/sensors/temperature";

bool isPlaying = false;
int selectedSong = 0;
unsigned long lastFirebaseCheck = 0;
unsigned long lastTempRead = 0;
const unsigned long firebaseInterval = 400;
const unsigned long tempInterval = 2000;
float lastTemperature = NAN;

void playMelody() {
  if (isPlaying) {
    return;
  }
  isPlaying = true;

  if (selectedSong < 0 || selectedSong >= songCount) {
    selectedSong = 0;
  }

  const Song& song = songs[selectedSong];
  Serial.printf("Odtwarzanie: %s\n", song.name);

  for (int thisNote = 0; thisNote < song.length; thisNote++) {
    int noteDuration = 1000 / song.durations[thisNote];

    if (song.notes[thisNote] != REST) {
      tone(buzzerPin, song.notes[thisNote], noteDuration);
    }

    delay((int)(noteDuration * 1.30));
    noTone(buzzerPin);
  }

  if (Firebase.ready()) {
    if (!Firebase.RTDB.setBool(&fbdo, playPath, false)) {
      Serial.printf("Błąd resetu play: %s\n", fbdo.errorReason().c_str());
    }
  }

  Serial.println("Melodia zakończona");
  isPlaying = false;
}

void readTemperature() {
  unsigned long now = millis();
  if (now - lastTempRead < tempInterval) {
    return;
  }
  lastTempRead = now;

  tempSensor.requestTemperatures();
  float celsius = tempSensor.getTempCByIndex(0);

  if (celsius <= DEVICE_DISCONNECTED_C + 0.5 || celsius > 125.0) {
    Serial.println("DS18B20: brak odczytu (sprawdź VCC, GND, pin 23 i rezystor 4.7k)");
    return;
  }

  lastTemperature = celsius;
  Serial.printf("Temperatura: %.2f °C\n", celsius);

  if (Firebase.ready()) {
    if (!Firebase.RTDB.setFloat(&fbdo, tempPath, celsius)) {
      Serial.printf("Błąd zapisu temperatury: %s\n", fbdo.errorReason().c_str());
    }
  }
}

void pollFirebase() {
  if (!Firebase.ready()) {
    return;
  }

  unsigned long now = millis();
  if (now - lastFirebaseCheck < firebaseInterval) {
    return;
  }
  lastFirebaseCheck = now;

  if (Firebase.RTDB.getInt(&fbdo, songPath)) {
    int id = fbdo.intData();
    if (id >= 0 && id < songCount) {
      selectedSong = id;
    }
  }

  if (Firebase.RTDB.getBool(&fbdo, playPath)) {
    if (fbdo.boolData()) {
      Serial.println("Żądanie zdalne — odtwarzanie");
      playMelody();
    }
  } else {
    Serial.printf("Błąd odczytu play: %s\n", fbdo.errorReason().c_str());
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(buzzerPin, OUTPUT);
  tempSensor.begin();
  tempSensor.setWaitForConversion(true);

  Serial.printf("DS18B20 na GPIO %d, znaleziono czujników: %d\n", tempPin, tempSensor.getDeviceCount());

  Serial.println("Test buzzera...");
  tone(buzzerPin, 880, 200);
  delay(250);
  noTone(buzzerPin);

  Serial.println("Łączenie z WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nPołączono z WiFi!");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  config.timeout.serverResponse = 8 * 1000;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.print("Oczekiwanie na Firebase");
  unsigned long startWait = millis();
  while (!Firebase.ready() && millis() - startWait < 15000) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();

  if (!Firebase.ready()) {
    Serial.println("UWAGA: Firebase nie jest gotowe — temperatura będzie tylko w Serial Monitor.");
  } else {
    if (!Firebase.RTDB.setBool(&fbdo, playPath, false)) {
      Serial.printf("Błąd zapisu play: %s\n", fbdo.errorReason().c_str());
    }
    if (!Firebase.RTDB.setInt(&fbdo, songPath, selectedSong)) {
      Serial.printf("Błąd zapisu song: %s\n", fbdo.errorReason().c_str());
    }
    Serial.println("Firebase - połączono");
  }

  Serial.println("Gotowe. Melodie sterujesz ze strony, temperatura z DS18B20 (GPIO 23).");
}

void loop() {
  readTemperature();
  pollFirebase();
  delay(20);
}
