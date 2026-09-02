# esp-epaper Hardware-Spezifikation (Software-Porting-Edition)

> Dieses Dokument ist für das Firmware-/Software-Porting der `followup`-
> Firmware auf ihr beibehaltenes `esp-epaper`-Board-Target gedacht. Es
> konzentriert sich auf den Hauptcontroller, die wichtigsten ICs, I2C-
> Adressen, die Stromversorgung und die ESP32-S3-GPIO-Definitionen.
>
> Das zugrunde liegende Board ist das Waveshare **ESP32-S3-ePaper-3.97**.
> Hersteller-Wiki: <https://docs.waveshare.com/ESP32-S3-ePaper-3.97>. Wo
> sich Hersteller-Wiki und Firmware widersprechen, ist die Firmware
> maßgeblich dafür, was der laufende Code tatsächlich ansteuert — siehe
> [Software-Porting-Hinweise](#9-software-porting-hinweise).

## 1. Systemübersicht

| Punkt | Spezifikation |
|---|---|
| Board-ID | `esp-epaper` (Waveshare ESP32-S3-ePaper-3.97) |
| Hauptcontroller | ESP32-S3-WROOM-1-N16R8 (`esp32s3`-Target) |
| Flash / PSRAM | 16 MB Flash / 8 MB Octal-PSRAM |
| Funk | 2,4 GHz WLAN (802.11 b/g/n) + Bluetooth 5 LE |
| Display | Waveshare 3,97"-E-Paper-Panel, `SSD1677`-Controller, `800x480` |
| Display-Bus | SPI (`SPI3_HOST`), E-Paper-Steuersignale |
| Audio-Codec | ES8311 über I2S + gemeinsam genutztes I2C |
| Audio-Verstärker | NS4150B (aktiviert über `GPIO39`) |
| Board-PCM-Format | `24 kHz`, Mono, 16-Bit |
| Speicher-Erweiterung | MicroSD über `SDMMC 4-Bit`-Modus |
| Energieverwaltung | AXP2101-PMIC (Wiki bezeichnet ihn als `TG28`) |
| RTC | PCF85063 |
| IMU | QMI8658 (6-Achsen) |
| Temperatur/Feuchtigkeit | SHTC3 (auf dem Board vorhanden; von der aktuellen Firmware nicht angesteuert) |
| USB | Natives `USB-OTG` über den USB-C-Anschluss des Boards |
| Gemeinsamer Steuerbus | I2C auf `GPIO41` (SDA) / `GPIO42` (SCL) |

## 2. Wichtige IC-Bauteilnummern

| Funktion | Bauteilnummer | I2C-Adresse | Software-relevante Hinweise |
|---|---|---|---|
| Hauptcontroller | ESP32-S3-WROOM-1-N16R8 | — | ESP-IDF-Target `esp32s3`; 16 MB Flash / 8 MB PSRAM |
| E-Paper-Controller | SSD1677 | — | 800x480-Panel; SPI-Kommandoschnittstelle |
| Audio-Codec | ES8311 | `0x30` | I2S-Audio + I2C-Steuerung; PA-Freigabe auf `GPIO39` |
| Audio-Verstärker | NS4150B | — | Lautsprecher-Endstufe; Freigabe über den Codec-PA-Pin `GPIO39` |
| Power-Management-IC | AXP2101 | `0x34` | Akkuladung + System-Spannungsschienen + VBUS-Ereignisse |
| RTC | PCF85063 | `0x51` | Echtzeituhr; Interrupt auf `GPIO45` |
| 6-Achsen-IMU | QMI8658 | `0x6B` / `0x6A` | Standard `0x6B`, Alternative `0x6A` (wird automatisch probiert); INT2 auf `GPIO40` |
| Temperatur-/Feuchtigkeitssensor | SHTC3 | `0x70` | Am gemeinsamen I2C-Bus; kein Treiber in der aktuellen Firmware |
| MicroSD | — | — | Steckplatz im `SDMMC 4-Bit`-Modus |

## 3. I2C-Peripherie-Adressen

Alle I2C-Peripheriegeräte teilen sich denselben Master-Bus (`GPIO41` SDA / `GPIO42` SCL).

| Peripheriegerät | Bauteilnummer | Adresse | Hinweise |
|---|---|---|---|
| Audio-Codec | ES8311 | `0x30` | `ES8311_CODEC_DEFAULT_ADDR` |
| PMIC | AXP2101 | `0x34` | `AXP2101_SLAVE_ADDRESS` |
| RTC | PCF85063 | `0x51` | `Pcf85063::kDefaultAddress` |
| 6-Achsen-IMU | QMI8658 | `0x6B` / `0x6A` | Firmware probiert zuerst `0x6B`, dann `0x6A` |
| Temperatur-/Feuchtigkeitssensor | SHTC3 | `0x70` | Laut Board-Design am Bus; von der aktuellen Firmware nicht angesprochen |

## 4. ESP32-S3-GPIO-Definitionen

### 4.1 E-Paper-Display (SPI3)

| Signal | GPIO | Richtung (aus Controller-Sicht) | Zweck |
|---|---:|---|---|
| `EPD_DC` | GPIO9 | Ausgang | E-Paper D/C-Auswahl |
| `EPD_CS` | GPIO10 | Ausgang | E-Paper SPI CS |
| `EPD_SCK` | GPIO11 | Ausgang | E-Paper SPI CLK |
| `EPD_MOSI` | GPIO12 | Ausgang | E-Paper SPI MOSI (DIN) |
| `EPD_RST` | GPIO46 | Ausgang | E-Paper-Reset |
| `EPD_BUSY` | GPIO3 | Eingang | E-Paper-Busy-Status |

### 4.2 Audio (I2S + Codec)

| Signal | GPIO | Richtung (aus Controller-Sicht) | Zweck |
|---|---:|---|---|
| `I2S_MCLK` | GPIO13 | Ausgang | Codec-Master-Takt |
| `I2S_BCLK` | GPIO14 | Ausgang | Codec-Bit-Takt |
| `I2S_WS` (LRCK) | GPIO47 | Ausgang | Codec Word-Select / LR-Takt |
| `I2S_DOUT` | GPIO48 | Ausgang | Codec-Datenausgang (Wiedergabe) |
| `I2S_DIN` | GPIO21 | Eingang | Codec-Dateneingang (Mikrofonaufnahme) |
| `CODEC_PA` | GPIO39 | Ausgang | Freigabe des Codec-Leistungsverstärkers |
| `CODEC_I2C_SDA` | GPIO41 | I/O | Codec-Steuerung I2C SDA (gemeinsamer Bus) |
| `CODEC_I2C_SCL` | GPIO42 | Ausgang | Codec-Steuerung I2C SCL (gemeinsamer Bus) |

### 4.3 I2C und Interrupts

| Signal | GPIO | Richtung (aus Controller-Sicht) | Zweck |
|---|---:|---|---|
| `I2C_SDA` | GPIO41 | I/O/Open-Drain | Gemeinsames I2C SDA (Codec/PMIC/RTC/IMU) |
| `I2C_SCL` | GPIO42 | Ausgang/Open-Drain | Gemeinsames I2C SCL (Codec/PMIC/RTC/IMU) |
| `PMIC_IRQ` | GPIO38 | Eingang | AXP2101-Interrupt (inkl. VBUS-Anstecken/-Abziehen) |
| `RTC_INT` | GPIO45 | Eingang | PCF85063-RTC-Interrupt |
| `QMI8658_INT2` | GPIO40 | Eingang | IMU-Interrupt 2 |

### 4.4 Tasten und Navigation

| Signal | GPIO | Richtung (aus Controller-Sicht) | Zweck |
|---|---:|---|---|
| `BOOT_BUTTON` | GPIO0 | Eingang | BOOT/Strap-Pin, zugleich die primäre Aktionstaste |
| `NAV_BUTTON_UP` | GPIO4 | Eingang | Navigation hoch |
| `NAV_BUTTON_FUNCTION` | GPIO5 | Eingang | Navigation Funktion / mittlere Taste |
| `NAV_BUTTON_DOWN` | GPIO6 | Eingang | Navigation runter |

### 4.5 MicroSD (SDMMC 4-Bit)

| Signal | GPIO | Richtung (aus Controller-Sicht) | Zweck |
|---|---:|---|---|
| `SD_D0` | GPIO15 | I/O | SD-Daten 0 |
| `SD_D1` | GPIO7 | I/O | SD-Daten 1 |
| `SD_D2` | GPIO8 | I/O | SD-Daten 2 |
| `SD_D3` | GPIO18 | I/O | SD-Daten 3 |
| `SD_CLK` | GPIO16 | Ausgang | SD-Takt |
| `SD_CMD` | GPIO17 | I/O | SD-Kommando |

## 5. Stromversorgung und Laden (AXP2101)

Der AXP2101-PMIC verantwortet die Systemstromversorgung, das Laden des Akkus und den USB-VBUS-Zustand. Das Firmware-Profil wird im `Pmic`-Konstruktor in
[`components/board_epaper/epaper_board.cc`](/Users/tieuvong/Development/followup/components/board_epaper/epaper_board.cc) konfiguriert.

### 5.1 Spannungsschienen

Das sind die PMIC-Ausgänge, die die Firmware beim Hochfahren aktiviert:

| Schiene | Zustand | Spannung | Hinweise |
|---|---|---|---|
| `DC1` | aktiviert | `3300 mV` | Haupt-System 3,3 V |
| `ALDO1` | aktiviert | `3300 mV` | 3,3 V Peripherie-LDO |
| `ALDO2` | aktiviert | `3300 mV` | 3,3 V Peripherie-LDO |
| `ALDO3` | aktiviert | `3300 mV` | 3,3 V Peripherie-LDO |
| Knopfzellen-/Backup-Akku | Laden aktiviert | `3000 mV` | Laden der Knopf-/Backup-Zelle (z. B. RTC-Backup) |
| System-Abschaltung | — | `2800 mV` | PMIC kappt die Systemversorgung unterhalb dieses Werts |

> Die **Zuordnung**, welche LDO-Schiene Display, Codec, SD oder RTC
> versorgt, ist eine Tatsache auf Schaltplan-Ebene und wird in der
> Firmware nicht deklariert — der Code setzt nur Spannungen und
> Freigaben. Die Zuordnung Schiene→Peripherie vor einer Umwidmung einer
> Schiene am Board-Schaltplan gegenprüfen.

### 5.2 Laden und VBUS

| Parameter | Wert |
|---|---|
| VBUS-Spannungsgrenze | `4,36 V` |
| VBUS-Strombegrenzung | `900 mA` |
| Ziel-Ladespannung | `4,2 V` |
| Konstanter Ladestrom | `400 mA` |
| Vorladestrom | `75 mA` |
| Abschaltstrom | `25 mA` |
| Temperatur-Schwellwert | `80 °C` |
| System-Abschaltspannung | `2800 mV` |
| Schwellwert für Akku-Warnung | `10 %` |
| Schwellwert für Akku-Abschaltung | `5 %` |

### 5.3 Abschalt-Modell

Abschalt-Modell (hybrid):

- Einschalten über den PMIC-Hardware-Tasten-Pfad
- Firmware-bestätigtes Herunterfahren vor dem PMIC-Power-off für den normalen In-App-Ablauf
- vom PMIC erzwungenes Herunterfahren nach `6 s` langem Drücken der Ein-/Austaste als erzwungener Fallback

Die PMIC-Interrupt-Leitung (`GPIO38`) zeigt auch das Anstecken/Abziehen des USB-Kabels an, genutzt für die Kabelerkennung im OTG-/Speicher-Modus und den automatischen Ausstieg zurück in den App-eigenen SD-Karten-Modus.

## 6. Schlafen und Aufwachen

Die Firmware nutzt **Light Sleep** (nicht Deep Sleep) bei Inaktivität, gesteuert vom Device-Sleep-Service. Der Aufwach-Pfad ist implementiert in
[`main/service_runtime/device_sleep_runtime.cc`](/Users/tieuvong/Development/followup/main/service_runtime/device_sleep_runtime.cc).

Ablauf vor `esp_light_sleep_start()`:

- Display-Refresh unterdrücken und das E-Paper-Panel schlafen legen (`SleepPanel()`)
- Board-Power-Interrupts fürs Aufwachen vorbereiten
- GPIO-Aufwach-Quellen scharfschalten, dann schlafen

### 6.1 Aufwach-Quellen

| Signal | GPIO | Auslöser | Zweck |
|---|---:|---|---|
| `PMIC_IRQ` (POWERKEY) | GPIO38 | Low-Pegel | Aufwachen durch Ein-/Austasten-Druck / PMIC-Ereignis |
| `BOOT_BUTTON` | GPIO0 | Low-Pegel | Aufwachen durch primäre Aktionstaste |

Beide werden mit `gpio_wakeup_enable(..., GPIO_INTR_LOW_LEVEL)` +
`esp_sleep_enable_gpio_wakeup()` scharfgeschaltet. Beim Verlassen liest die Firmware beide Pin-Pegel, um die Aufwach-Ursache zuzuordnen und das dadurch ausgelöste Fehl-Tastenereignis zu unterdrücken, ruft dann bei beiden `gpio_wakeup_disable()` auf und stellt die normalen Power-Interrupts wieder her.

### 6.2 Hinweise

- Das ist **Light Sleep**, es bleibt also RAM-/Peripheriezustand erhalten; in dieser Firmware gibt es keinen EXT1-/RTC-Domain-`esp_sleep_enable_ext1_wakeup_io()`-Deep-Sleep-Pfad.
- Aufwachen der IMU bei Bewegung ist im QMI8658-Treiber vorhanden, aber in diesem Board-Profil **deaktiviert** (`QMI8658_ENABLE_WAKE_ON_MOTION = false`) — Bewegung ist also keine Aufwach-Quelle.
- Die RTC-Alarmbehandlung ist ein Firmware-Timer-Polling, das während des Schlafs deaktiviert ist — die RTC (`GPIO45`) wird nicht als Light-Sleep-Aufwach-GPIO genutzt.

## 7. Strap-Pins

Die Strap-Pins des ESP32-S3 bergen ein Startup-Risiko; mehrere davon werden auf diesem Board als funktionale Signale wiederverwendet und müssen beim Reset einen Boot-sicheren Standardpegel halten:

| GPIO | Strap-Rolle | Nutzung auf dem Board | Vorsicht |
|---|---:|---|---|
| `GPIO0` | Boot-/Download-Auswahl | Primäre Aktionstaste (auch POWERKEY-Aufwachen) | Muss beim Reset high sein für normalen Boot |
| `GPIO3` | JTAG-Quellenauswahl | `EPD_BUSY`-Eingang | Standardpegel beim Start Boot-sicher halten |
| `GPIO45` | VDD_SPI-Spannung | `RTC_INT`-Eingang | Während des Resets nicht aktiv schalten |
| `GPIO46` | Boot-/ROM-Meldungen | `EPD_RST`-Ausgang | Während des Resets nicht aktiv schalten |

## 8. USB-/OTG-Speicher

- Der OTG-Modus nutzt den nativen ESP32-S3-USB-Device-Pfad, um die SD-Karte als USB-Massenspeicher bereitzustellen
- der Nutzer wechselt über die Aktion `Enable OTG` auf der Einstellungsseite in den USB-Speicher-Modus
- während der Aktivierung gehört die SD-Karte dem Host, und die App blockiert die normale Navigation hinter einem Speicher-Modal
- kurzer Druck der Ein-/Austaste, Betätigung der BOOT-Taste oder Abziehen des USB-Kabels fordern den Ausstieg zurück in den App-gemounteten SD-Karten-Modus an

## 9. Software-Porting-Hinweise

| Punkt | Hinweise |
|---|---|
| Quelle der Wahrheit | Pin-Werte stammen aus `epaper_board_config.h`; die Audio-Pins in `docs/hardware-reference.md` nicht als aktuell behandeln — diese Datei listet ein älteres I2S-Mapping (`WS`/`DOUT`/`DIN` auf GP15/GP16/GP21). Der Config-Header nutzt `WS=GPIO47`, `DOUT=GPIO48`, `DIN=GPIO21`, `MCLK=GPIO13`, `BCLK=GPIO14`. |
| Gemeinsamer I2C-Bus | Codec, PMIC, RTC und IMU teilen sich einen Master-Bus auf `GPIO41`/`GPIO42`. |
| IMU-Adresse | QMI8658 erkennt sich automatisch: zuerst `0x6B` probieren, dann `0x6A` als Fallback. |
| Strap-Pins | `GPIO0/3/45/46` sind Strap-Pins, die als funktionale Signale wiederverwendet werden — siehe [Strap-Pins](#7-strap-pins). |
| SD-Modus | `SDMMC 4-Bit` (nicht SPI); alle vier Datenleitungen angeschlossen (`D0`–`D3`). |
| Audio-Abtastrate | Codec läuft mit `24 kHz` Mono/16-Bit; der Gemini-Upload-Pfad tastet Mikrofon-Audio softwareseitig auf `16 kHz` herunter. |
| PMIC-Bezeichnung | Das Waveshare-Wiki listet den PMIC als `TG28`; der Firmware-Treiber zielt auf einen AXP2101-kompatiblen PMIC unter I2C `0x34`, und das ist es, was tatsächlich funktioniert. `0x34` / AXP2101 als maßgeblich behandeln. |
| SHTC3-Sensor | Auf dem Board vorhanden (gemeinsames I2C, `0x70`), aber die aktuelle Firmware liefert keinen SHTC3-Treiber mit — vor Verlassen auf Temperatur/Feuchtigkeit erst einen hinzufügen. |
| Anschlüsse | Akku, Lautsprecher und RTC-Backup-Akku nutzen MX1.25-Stecker (laut Hersteller-Wiki); USB-C wird zum Flashen/Loggen und für natives USB-OTG genutzt. |

## 10. Build und Flash

```bash
source $IDF_PATH/export.sh
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

Release-Hilfsskript:

```bash
python3 scripts/release.py esp-epaper
```
