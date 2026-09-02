# Auto Sleep

Auto Sleep nutzt die QMI8658-IMU zur Erkennung von Inaktivität, geht zuerst
in den E-Paper-Display-Sleep und später in den ESP32-S3-Light-Sleep. Die
aktuelle Implementierung ist auf dieser Hardware bewährt und nutzt bewusst
direktes IMU-Polling statt FIFO oder IMU-Interrupts.

## Runtime-Zuständigkeit

Auto Sleep ist zwischen Policy und Hardware-Runtime-Code aufgeteilt:

- `device_sleep_service` besitzt den Sleep-Zustandsautomaten, die
  Inaktivitäts-Timer, die Timeout-Validierung, den Blocker-Zustand und die
  Übergangs-Events.
- `main/device_sleep_runtime.cpp` besitzt produktspezifisches
  Hardware-Verhalten: IMU-Polling, Display-Sleep-Kommandos, Eintritt in den
  ESP-Light-Sleep, `ACTION`-Wake-Einrichtung und Blocker-Aggregation.
- `app_shell` bleibt Orchestrator. Es stellt Einstellungen bereit, leitet
  Nutzer-Aktivität weiter, liefert app-eigenen Blocker-Zustand und startet
  die Runtime.

## Stufen

Das Gerät durchläuft drei Stufen:

- `awake`: normales App-Verhalten.
- `display_sleeping`: das E-Paper-Panel hat auf einen leeren Bildschirm
  aufgefrischt und ist dann in den Panel-Sleep gegangen.
- `light_sleeping`: das E-Paper-Panel hat auf einen leeren Bildschirm
  aufgefrischt, ist in den Panel-Sleep gegangen, und der ESP32-S3 ist in
  `esp_light_sleep_start()` gegangen.

Bewegung oder Nutzer-Interaktion weckt das Display aus `display_sleeping`.
`ACTION` / `GPIO0` oder der PMIC-Interrupt weckt den ESP32-S3 aus
`light_sleeping`.

## IMU-Inaktivitätserkennung

Die Runtime samplet `imu_service::ReadSample(...)` alle `200 ms` und
vergleicht die neueste Beschleunigungssensor-Probe mit der vorherigen.
Beschleunigungssensor-Werte werden vor der Schwellenwert-Anwendung von `g`
in `mg` umgerechnet.

Aktuell validierte Schwellenwerte:

- Bewegung beginnt, wenn die Achsen-Delta-Summe mindestens `60 mg` beträgt.
- Bewegung beginnt auch, wenn das größte Einzelachsen-Delta mindestens
  `25 mg` beträgt.
- Stillstand erfordert, dass die Achsen-Delta-Summe bei oder unter `20 mg`
  bleibt.
- Stillstand erfordert außerdem, dass das größte Einzelachsen-Delta bei
  oder unter `8 mg` bleibt.
- Keine-Bewegung wird erst nach einem durchgehenden `2 s`-Stillstandsfenster
  scharf geschaltet.

Diese Werte wurden am Gerät validiert. Normale Tischvibration erforderte
keine Schwellenwert-Änderungen, und das Aufnehmen des Geräts weckt das
Display umgehend.

## Display-Sleep

Nachdem das konfigurierte Display-Sleep-Timeout während einer
Keine-Bewegung-Phase abgelaufen ist, bittet die Runtime `display_service`,
in den Display-Sleep zu gehen.

Die Display-Sequenz ist:

1. Das E-Paper-Panel auf einen leeren Bildschirm auffrischen.
2. Warten, bis das E-Paper-Auffrischen fertig ist.
3. Das E-Paper-Panel in den Sleep versetzen.
4. Das Panel im Schlaf lassen, bis Bewegung oder Nutzer-Interaktion es weckt.

Bewegung oder Nutzer-Interaktion weckt das Display und stellt die leere
App-Oberfläche mit einem vollständigen Auffrischen wieder her.

## Light-Sleep

Nachdem das konfigurierte Light-Sleep-Timeout während derselben
Keine-Bewegung-Phase abgelaufen ist, geht die Runtime in den
ESP32-S3-Light-Sleep.

Es gibt auf diesem Board keinen Power-Latch zu schützen -- der AXP2101 hält
die Spannungsschienen während des Light-Sleep von sich aus, was das ganze
`PWR_HOLD`/`PWR_LOCK`-Sleep-GPIO-Problem des Sticky entfällt.

Ein Schutz bleibt trotzdem bestehen: `ACTION` / `GPIO0` ist sowohl eine
Light-Sleep-Wake-Quelle als auch die Aufnahme-Taste der App. Die Runtime
scharft eine Wake-Only-Unterdrückung, bevor sie `esp_light_sleep_start()`
aufruft, damit der weckende Tastendruck nicht in `app_shell` durchsickern
und eine Aufnahme scharf schalten kann. Die Unterdrückung wird durch das
passende Release-/Klick-Event nach dem Wecken aufgehoben, oder durch ein
Timeout, falls dieses Event nie eintrifft.

Die AXP2101-Interrupt-Leitung (`GPIO38`) ist die zweite Wake-Quelle, worüber
ein `PWR`-Druck das Board weckt.

Die Light-Sleep-Sequenz ist:

1. `ACTION` / `GPIO0` als Eingang mit Pull-Up konfigurieren.
2. Warten, bis `ACTION` / `GPIO0` losgelassen/high ist.
3. `ACTION` / `GPIO0` und den PMIC-IRQ / `GPIO38` als Active-Low
   `gpio_wakeup_enable`-Light-Sleep-Wake-Quellen scharf schalten (nicht EXT1:
   dieses Board nutzt den Light-Sleep-GPIO-Wake-Pfad, der die Pads auf dem
   digitalen Peripheriegerät belässt).
4. Den Button-Service-Polling-Timer aussetzen, damit der Uhr-Sprung des
   Light-Sleep keinen Schwall verpasster Ticks nachspielen und die
   Klick-Klassifikation beim Aufwachen zerstören kann.
5. Wake-Only-`ACTION`-Event-Unterdrückung scharf schalten.
6. Das E-Paper-Panel auf einen leeren Bildschirm auffrischen.
7. Warten, bis das E-Paper-Auffrischen fertig ist, und das Panel in den
   Sleep versetzen.
8. `esp_light_sleep_start()` aufrufen.
9. Beim Aufwachen die GPIO-Wake-Quellen entschärfen, das `ACTION`-Pad
   wiederherstellen und das Button-Polling fortsetzen, bevor irgendetwas
   Langsames läuft.
10. Den Wake-Übergang sofort festschreiben, ohne ein zweites Wake-Event
    einzureihen.
11. Das weckende Power-Button-Event als Wake-Only konsumieren.
12. Das Display mit einem erzwungenen vollständigen Auffrischen
    wiederherstellen, selbst wenn der Software-Zustand bereits zurück zu
    Wach gewechselt ist.

Normale Power-Button-Interaktionen im Wach-Zustand bleiben außerhalb des
Light-Sleep-Wake-Pfads verfügbar. Aktuell bedeutet das: ein kurzer Druck
der `PWR`-Taste schaltet den Sperrbildschirm um, während ein ~1s-Halten von
`PWR` den Herunterfahren-Bestätigungsdialog öffnet. Beide kommen als
AXP2101-Interrupts statt als GPIO-Button-Events an und erfordern dann eine
explizite Bestätigung über das globale Herunterfahren-Modal.

## Sleep-Blocker

Auto Sleep wird während Abläufen blockiert, bei denen Schlafen aktive
Arbeit unterbrechen oder den Hardware-Zustand schwerer nachvollziehbar
machen würde.

Aktuelle Blocker:

- Aufnahme aktiv
- Aufnahme scharf geschaltet
- Aufnahme wird gespeichert oder exportiert
- Herunterfahren steht bevor, inklusive Herunterfahren-Bestätigungsdialog
- Display-Auffrischen aktiv
- app-deklarierte Speicher-Schreibaktivität
- WLAN-Access-Point-Einrichtungsmodus
- SNTP-Zeit-Synchronisierung läuft

Reine USB-Stromversorgung blockiert Auto Sleep nicht.

Während der SD-Formatierung hebt `storage_service::IsWriteBusy()` den
`storage_write`-Blocker. Das hält den Sleep-Zustandsautomaten davon ab,
während der Formatierung in den Display-Sleep oder Light-Sleep zu gehen.
IMU-Bewegungs-Polling läuft währenddessen weiter, liest aber den QMI8658
über den geteilten Sensor-I2C-Bus und konkurriert nicht direkt mit dem
geteilten SPI-Bus, den MicroSD und das E-Paper-Panel nutzen.
Bewegungs-Logs während der Formatierung sind daher zu erwarten und sind
für sich genommen kein Beleg dafür, dass der SD-Formatierungspfad
unterbrochen wird.

## Konfiguration

Die Build-Time-Einstellungen liegen unter `Folloup Settings`:

- `CONFIG_FOLLOWUP_AUTO_SLEEP_DISPLAY_SLEEP_TIMEOUT_SECONDS`
- `CONFIG_FOLLOWUP_AUTO_SLEEP_LIGHT_SLEEP_TIMEOUT_SECONDS`

Aktuelle Standardwerte:

- Display-Sleep: `180 s` (3 Minuten)
- Light-Sleep: `1800 s` (30 Minuten)

Einen der beiden Timeouts auf `0` setzen, um diese Stufe zu deaktivieren.
Wenn beide Stufen aktiviert sind, muss das Light-Sleep-Timeout größer oder
gleich dem Display-Sleep-Timeout sein.

## Logging

Die Runtime loggt beim Start die aufgelösten Auto-Sleep-Einstellungen,
Bewegungs- und Keine-Bewegung-Erkennung, Blocker-Änderungen,
Stufen-Übergänge, Display-Sleep-/Wake-Aktionen, Light-Sleep-Eintritt und
die Light-Sleep-Wake-Ursache nach dem Light-Sleep. Diese Logs wurden für
die Validierung am Gerät genutzt und sollten stabil genug für künftige
Hardware-Tests bleiben.

## Zurückgestellter FIFO- und Shared-ISR-Plan

FIFO-gestütztes Sampling und IMU-Interrupt-Behandlung sind bewusst
zurückgestellt. Der aktuelle `200 ms`-Polling-Ansatz ist einfach,
debugbar, reaktionsschnell genug und erfordert kein Teilen von `GPIO7`
zwischen zwei Interrupt-Quellen.

FIFO sollte nur überdacht werden, wenn eines davon eintritt:

- Polling verbraucht zu viel Strom
- kurze Bewegungsschübe werden verpasst
- I2C-Verkehr wird zum Problem
- eine glattere Bewegungshistorie wird für ein künftiges Feature gebraucht

IMU-Interrupt-Behandlung sollte nur überdacht werden, wenn eines davon
eintritt:

- Bewegungs-Wake muss aus dem ESP-Light-Sleep heraus funktionieren
- die Aufnehmen-Wake-Latenz muss niedriger sein als das Polling-Intervall
- Hardware-Messungen zeigen, dass Polling ersetzt werden sollte
- ein anderes Feature braucht IMU-Aktivität-, Inaktivität-,
  FIFO-Watermark-, Orientierungs-, Tap- oder Data-Ready-Interrupts

Die QMI8658-IMU teilt sich den Sensor-I2C-Bus mit dem PMIC und der RTC.
Falls der IMU-Interrupt-Pfad später hinzugefügt wird, sollte weder
`power_service` noch `imu_service` `GPIO7` unabhängig beanspruchen. Einen
gemeinsamen Leitungs-Besitzer hinzufügen, der:

- den `GPIO7`-ISR besitzt
- den ISR minimal hält
- alle I2C-Arbeit an einen Task delegiert
- den PMIC-Interrupt-Zustand prüft und loggt, ohne bestehende Diagnostik
  zu verlieren
- die IMU-Interrupt-Quelle oder den FIFO-Zustand prüft und loggt
- identifiziert, welche Quelle die geteilte Leitung ausgelöst hat
- das aktuelle Auto-Sleep-Verhalten bewahrt, bis der neue Interrupt-Pfad
  sich bewährt hat
