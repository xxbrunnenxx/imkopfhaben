#ifndef JOINT_TRACKER_SERVICE_H_
#define JOINT_TRACKER_SERVICE_H_

#include <cstdint>
#include <vector>

// Der 420-Track: zaehlt die Joints des laufenden Tages und fuehrt ein
// rollierendes Protokoll der vergangenen Tage. Der Besitzer ist Patient mit
// einem taeglichen Richtwert (kDailyGoal); ueber den Wert hinaus wird nur
// markiert, nie gesperrt.
//
// Persistenz liegt in NVS: der heutige Zaehler mit seinem Datum plus ein
// Protokoll-Blob. Beim ersten Zugriff eines neuen Kalendertags wandert der
// gestrige Stand ins Protokoll und der Zaehler faellt auf 0 (Reset um
// Mitternacht, Ortszeit).
namespace joint_tracker_service {

// Fester Tagesrichtwert. Bewusst eine Konstante: der Besitzer hat 4 als Limit
// vorgegeben, einstellbar ist es (noch) nicht.
constexpr int kDailyGoal = 4;

// Wie viele vergangene Tage das Protokoll behaelt.
constexpr int kHistoryDays = 90;

struct DayCount {
    int32_t day = 0;   // Kalendertag als YYYYMMDD (Ortszeit)
    int16_t count = 0; // Joints an diesem Tag
};

// Laedt Zaehler, Datum und Protokoll aus NVS. Muss einmal nach nvs_flash_init()
// laufen. Fuehrt gleich den faelligen Tageswechsel aus (siehe RolloverIfNeeded).
void Load();

// Heutiger Zaehler (>= 0). Loest bei Bedarf den Tageswechsel aus.
int GetTodayCount();

// Tagesrichtwert (kDailyGoal), als Funktion fuer kuenftige Einstellbarkeit.
int GetDailyGoal();

// Erhoeht/senkt den heutigen Zaehler um eins. Unter 0 wird nicht gesenkt; nach
// oben gibt es keine Grenze (ueber kDailyGoal ist erlaubt, nur markiert).
// Schreibt nach NVS. Gibt den neuen Wert zurueck.
int Increment();
int Decrement();

// Prueft, ob ein neuer Kalendertag begonnen hat, und holt den Tageswechsel
// dann nach: gestrigen Stand ins Protokoll, Zaehler auf 0. Wird von den
// Gettern/Mutatoren selbst aufgerufen; oeffentlich fuer den periodischen
// Aufruf aus einem Hintergrund-Task (damit die Startseite auch ohne
// Tastendruck ueber Mitternacht zurueckspringt). Gibt true zurueck, wenn ein
// Wechsel stattfand.
bool RolloverIfNeeded();

// Kopie des Protokolls, aeltester zuerst, inklusive des heutigen Tages.
std::vector<DayCount> GetHistory();

}  // namespace joint_tracker_service

#endif  // JOINT_TRACKER_SERVICE_H_
