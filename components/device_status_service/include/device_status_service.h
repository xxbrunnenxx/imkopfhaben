#ifndef DEVICE_STATUS_SERVICE_H_
#define DEVICE_STATUS_SERVICE_H_

#include <string>

#include "esp_err.h"

// Sammelt die Kennzahlen fuer die Startseite an einer Stelle: was das Geraet
// von sich selbst weiss (eigene IP, interne Temperatur) und was der Brain-Pi
// ueber /api/status meldet (Uptime, CPU-Temperatur, Last). Ein Refresh macht
// einen blockierenden HTTP-Aufruf -- deshalb nur aus einem Worker-Task rufen,
// nie aus einem UI-/Eingabe-Task.
namespace device_status_service {

struct Snapshot {
    // Geraeteseite (immer sofort verfuegbar, ohne Netz).
    bool wifi_connected = false;
    std::string device_ip = {};   // leer, wenn nicht verbunden
    std::string ssid = {};
    int rssi = 0;

    bool esp_temp_valid = false;
    float esp_temp_celsius = 0.0f;

    // Brain-Seite (aus /api/status; nur gueltig, wenn brain_reachable).
    std::string brain_host = {};  // z. B. "kraken.local:8000"
    std::string brain_ip = {};    // aufgeloeste Adresse, leer wenn unbekannt
    bool brain_reachable = false;
    int brain_http_status = 0;

    bool brain_uptime_valid = false;
    double brain_uptime_seconds = 0.0;
    bool pi_uptime_valid = false;
    double pi_uptime_seconds = 0.0;
    bool pi_temp_valid = false;
    float pi_temp_celsius = 0.0f;
    bool cpu_load_valid = false;
    float cpu_load_percent = 0.0f;
    bool ram_valid = false;
    float ram_percent = 0.0f;
};

// Installiert den internen Temperatursensor. Ein Fehler ist nicht fatal:
// die Temperatur bleibt dann ungueltig, der Rest funktioniert weiter.
esp_err_t Init();

// Frischt den Schnappschuss auf: liest WLAN-Status und interne Temperatur,
// leitet die Brain-URL aus der Transkriptions-URL ab und holt /api/status.
// Blockierend (HTTP) -- aus einem Worker-Task rufen.
void Refresh();

// Der zuletzt ermittelte Stand. Thread-sicher, kopiert.
Snapshot GetSnapshot();

}  // namespace device_status_service

#endif  // DEVICE_STATUS_SERVICE_H_
