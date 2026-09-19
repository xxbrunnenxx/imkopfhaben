#ifndef ARCHIVE_PORTAL_H_
#define ARCHIVE_PORTAL_H_

#include "esp_http_server.h"

// Lesender Blick von aussen auf das, was auf der SD-Karte liegt: die
// Aufnahmen samt Transkript und die zuletzt erzeugten Zusammenfassungen.
//
// Warum es das gibt: bis hierher war der einzige Weg an den Inhalt das
// E-Paper-Display am Geraet selbst -- ein paar Zeilen auf einmal, nicht
// kopierbar, nicht vergleichbar. Wer pruefen wollte, ob eine
// Zusammenfassung zu den Aufnahmen passt, musste sie abtippen. Diese
// Routen liefern denselben Bestand, den die Seiten am Geraet anzeigen,
// als JSON ueber dieselbe Weboberflaeche, die schon fuer WLAN, Zeitzone
// und den KI-Server laeuft.
//
// Lesend, mit einer Ausnahme: /api/archive/summarize stoesst einen
// Zusammenfass-Lauf an. Das aendert keine Aufnahme, sondern startet genau
// das, was auch der Knopf am Geraet startet -- und macht die Pruefzeile
// "eine volle Zusammenfassung geht durch" ohne einen Menschen am Geraet
// nachvollziehbar. Loeschen, Umtaggen und Formatieren bleiben draussen:
// die gehoeren ans Geraet, wo der Besitzer sieht, was er anfasst.
namespace archive_portal {

// Haengt die Routen in den laufenden Portal-Server:
//   GET /api/archive/recordings  -- alle Aufnahmen mit Metadaten
//                                   (?transcripts=0 laesst die Texte weg)
//   GET /api/archive/summaries   -- die gespeicherten Zusammenfassungen
//   POST /api/archive/summarize  -- stoesst einen Lauf an (?kind=todos|notes)
void RegisterPortalRoutes(httpd_handle_t server);

}  // namespace archive_portal

#endif  // ARCHIVE_PORTAL_H_
