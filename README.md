# ESP32 Buzzer + DS18B20

Projekt na ESP32: zdalne odtwarzanie melodii na buzzerze ze strony WWW oraz podgląd temperatury z czujnika DS18B20. Komunikacja idzie przez **Firebase Realtime Database**.

## Funkcje

- wybór i odtwarzanie melodii z przeglądarki (Prosta melodia, Sto lat, Gwiazdeczka, Mario, Jingle Bells)
- odczyt temperatury DS18B20 (zasilanie normalne, nie parasite) co ok. 2 s
- podgląd temperatury i stanu odtwarzania na stronie `index.html`

## Sprzęt

| Element | Pin ESP32 |
| --- | --- |
| Buzzer (sygnał) | GPIO 18 |
| DS18B20 DATA | GPIO 23 |

### DS18B20 — tryb normalny

- **VCC** → 3.3 V
- **GND** → GND
- **DATA** → GPIO 23
- rezystor **4.7 kΩ** między DATA a 3.3 V

Buzzer: drugi przewód do GND (albo do VCC, zależnie od modułu; aktywny buzzer vs pasywny — szkic używa `tone()`, więc potrzebny jest **buzzer pasywny**).

## Pliki

| Plik | Opis |
| --- | --- |
| `sketch.ino` | firmware ESP32 (Wi‑Fi, Firebase, buzzer, DS18B20) |
| `index.html` | panel web (melodie + temperatura) |
| `pitches.h` | definicje częstotliwości nut |

Arduino IDE zwykle wymaga, żeby plik `.ino` nazywał się tak jak folder (`esp_buzzer.ino`). Jeśli IDE zgłosi błąd nazwy, zmień `sketch.ino` na `esp_buzzer.ino`.

## Biblioteki (Arduino IDE)

Zainstaluj przez **Szkic → Dołącz bibliotekę → Zarządzaj bibliotekami**:

- **Firebase Arduino Client Library for ESP8266 and ESP32** (`Firebase_ESP_Client.h`, autor Mobizt)
- **OneWire**
- **DallasTemperature**

Dodatkowo: wsparcie płytki **ESP32** (Arduino-ESP32).

## Konfiguracja

### 1. Firmware (`sketch.ino`)

Uzupełnij na górze pliku:

```cpp
#define WIFI_SSID "nazwa_sieci"
#define WIFI_PASSWORD "haslo_wifi"

#define API_KEY "klucz_api_firebase"
#define DATABASE_URL "https://PROJEKT-default-rtdb.REGION.firebasedatabase.app/"
#define USER_EMAIL "email@konta.firebase"
#define USER_PASSWORD "haslo_konta"
```

ESP loguje się do Firebase **e-mailem i hasłem** (Authentication → Email/Password).

### 2. Strona (`index.html`)

W `firebaseConfig` wstaw dane swojego projektu (apiKey, authDomain, databaseURL, projectId, appId).

Strona loguje się **anonimowo**. W Firebase Console włącz:

**Authentication → Sign-in method → Anonymous** (oraz Email/Password dla ESP).

### 3. Reguły Realtime Database

Odczyt i zapis wymagają zalogowanego użytkownika, na przykład:

```json
{
  "rules": {
    ".read": "auth != null",
    ".write": "auth != null"
  }
}
```

## Ścieżki w Firebase

| Ścieżka | Typ | Znaczenie |
| --- | --- | --- |
| `board1/buzzer/play` | bool | `true` = zagraj wybraną melodię; ESP po skończeniu ustawia `false` |
| `board1/buzzer/song` | int | indeks utworu (0–4) |
| `board1/sensors/temperature` | float | temperatura w °C |

## Uruchomienie

1. Podłącz buzzer i DS18B20 jak w tabeli.
2. Uzupełnij Wi‑Fi i Firebase w `sketch.ino` oraz `index.html`.
3. Wgraj szkic na ESP32.
4. Otwórz **Monitor portu szeregowego** (115200). Powinny pojawić się: test buzzera, liczba znalezionych DS18B20, połączenie z Wi‑Fi i Firebase.
5. Otwórz `index.html` w przeglądarce, wybierz utwór i kliknij **Odtwórz melodię**.

## Jak to działa

1. Strona zapisuje `song` i ustawia `play = true`.
2. ESP co ~400 ms odczytuje te pola; gdy `play` jest `true`, gra melodię na GPIO 18 i zeruje flagę.
3. Równolegle ESP mierzy temperaturę i zapisuje ją do `board1/sensors/temperature`.
4. Strona nasłuchuje tych węzłów i odświeża UI na żywo.

## Problemy

- **Strona: „ładowanie” / błąd logowania** — włącz Anonymous Auth; sprawdź `firebaseConfig` i reguły bazy.
- **Brak odczytu DS18B20** — VCC 3.3 V, wspólne GND, rezystor 4.7 kΩ na linii DATA, pin 23. W Serial Monitor: `znaleziono czujników: 1`.
- **Brak dźwięku** — GPIO 18, buzzer **pasywny**, wspólne GND.
- **ESP nie łączy się z Firebase** — Email/Password, poprawny `API_KEY` i `DATABASE_URL` (z regionem).
