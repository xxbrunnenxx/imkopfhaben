// Minimaler NVS-Stub: In-Memory-Key-Value je Namespace, genug fuer den
// joint_tracker_service (i32 + blob). KEIN echtes NVS, nur damit die ECHTE
// joint_tracker_service.cpp unveraendert host-baubar ist.
#ifndef NVS_STUB_H
#define NVS_STUB_H
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_NVS_NOT_FOUND 0x1102
#define ESP_FAIL -1

typedef enum { NVS_READONLY, NVS_READWRITE } nvs_open_mode_t;
typedef int nvs_handle_t;

inline const char* esp_err_to_name(esp_err_t e) { return e == ESP_OK ? "ESP_OK" : "ESP_ERR"; }

struct NvsStore {
    std::map<std::string, int32_t> i32;
    std::map<std::string, std::vector<uint8_t>> blob;
};
inline std::map<std::string, NvsStore>& nvs_all() { static std::map<std::string, NvsStore> m; return m; }
inline std::map<nvs_handle_t, std::string>& nvs_handles() { static std::map<nvs_handle_t, std::string> m; return m; }

inline esp_err_t nvs_open(const char* ns, nvs_open_mode_t, nvs_handle_t* out) {
    static nvs_handle_t next = 1;
    // READONLY auf noch nie geoeffneten Namespace -> NOT_FOUND (wie echt)
    if (nvs_all().find(ns) == nvs_all().end()) {
        // Beim ersten Lesen ist der Namespace leer; echtes NVS liefert dann
        // NOT_FOUND. Wir bilden das nur fuer den READONLY-Erstzugriff nach,
        // indem der Service selbst NOT_FOUND toleriert. Zur Einfachheit legen
        // wir ihn hier an und liefern OK; der Service liest dann 0 Keys.
        nvs_all()[ns];
    }
    nvs_handle_t h = next++;
    nvs_handles()[h] = ns;
    *out = h;
    return ESP_OK;
}
inline esp_err_t nvs_get_i32(nvs_handle_t h, const char* key, int32_t* v) {
    auto& s = nvs_all()[nvs_handles()[h]];
    auto it = s.i32.find(key);
    if (it == s.i32.end()) return ESP_ERR_NVS_NOT_FOUND;
    *v = it->second; return ESP_OK;
}
inline esp_err_t nvs_set_i32(nvs_handle_t h, const char* key, int32_t v) {
    nvs_all()[nvs_handles()[h]].i32[key] = v; return ESP_OK;
}
inline esp_err_t nvs_get_blob(nvs_handle_t h, const char* key, void* out, size_t* len) {
    auto& s = nvs_all()[nvs_handles()[h]];
    auto it = s.blob.find(key);
    if (it == s.blob.end()) return ESP_ERR_NVS_NOT_FOUND;
    if (out == nullptr) { *len = it->second.size(); return ESP_OK; }
    size_t n = it->second.size() < *len ? it->second.size() : *len;
    std::memcpy(out, it->second.data(), n); *len = n; return ESP_OK;
}
inline esp_err_t nvs_set_blob(nvs_handle_t h, const char* key, const void* data, size_t len) {
    auto& s = nvs_all()[nvs_handles()[h]];
    const uint8_t* p = static_cast<const uint8_t*>(data);
    s.blob[key].assign(p, p + len); return ESP_OK;
}
inline esp_err_t nvs_erase_key(nvs_handle_t h, const char* key) {
    nvs_all()[nvs_handles()[h]].blob.erase(key); return ESP_OK;
}
inline esp_err_t nvs_commit(nvs_handle_t) { return ESP_OK; }
inline void nvs_close(nvs_handle_t h) { nvs_handles().erase(h); }

#endif
