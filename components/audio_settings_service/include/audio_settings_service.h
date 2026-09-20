#ifndef AUDIO_SETTINGS_SERVICE_H_
#define AUDIO_SETTINGS_SERVICE_H_

class AudioCodec;

// Persistente Ausgabelautstaerke fuer den ES8311-Codec. Der Wert ueberlebt
// den Neustart in NVS; beim Boot wendet waveshare_board::GetAudioCodec() ihn
// an. Die Advanced-Seite verstellt ihn in festen Stufen (0/25/50/75/100),
// wobei 0 den Ausgang stummschaltet.
namespace audio_settings_service {

// Erlaubte Stufen, aufsteigend. Index 2 (=50) ist der Auslieferungswert.
constexpr int kVolumeSteps[] = {0, 25, 50, 75, 100};
constexpr int kVolumeStepCount = 5;
constexpr int kDefaultVolume = 50;

// Laedt den gespeicherten Wert aus NVS (Default 50, wenn nichts steht). Muss
// einmal nach nvs_flash_init() laufen, bevor GetVolume() belastbar ist.
void Load();

// Aktuelle Lautstaerke in Prozent (0..100), immer eine der kVolumeSteps.
int GetVolume();

// Aktueller Index in kVolumeSteps (0..kVolumeStepCount-1).
int GetVolumeIndex();

// Setzt die Lautstaerke auf die Stufe am Index (wird eingeklemmt) und schreibt
// sie nach NVS. Der Dienst haelt bewusst keinen Codec-Zeiger; das Anwenden auf
// die Hardware macht der Aufrufer (Board beim Boot, Advanced-Seite bei jeder
// Aenderung) ueber ApplyVolumeToCodec(). Gibt true zurueck, wenn sich der Wert
// geaendert hat.
bool SetVolumeIndex(int index);

// Wendet die aktuelle Lautstaerke auf den uebergebenen Codec an: 0 % schaltet
// stumm (SetOutputMuted(true)), jede andere Stufe hebt die Stummschaltung auf.
void ApplyVolumeToCodec(AudioCodec* codec);

}  // namespace audio_settings_service

#endif  // AUDIO_SETTINGS_SERVICE_H_
