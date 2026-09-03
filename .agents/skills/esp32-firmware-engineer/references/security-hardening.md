# Referenz: Sicherheits-Härtung

## Überblick der Sicherheits-Funktionen

| Funktion | Wo konfiguriert | Umkehrbar? | Für Produktion erforderlich? |
|---|---|---|---|
| Secure Boot v2 | eFuse + sdkconfig | Nein (eFuse-Brennung) | Ja, für signierte Feldgeräte |
| Flash-Verschlüsselung | eFuse + sdkconfig | Nein (nur im Development-Modus) | Ja, für sensible Daten |
| NVS-Verschlüsselung | sdkconfig + Key-Partition | Ja (Key löschbar) | Falls NVS Geheimnisse enthält |
| JTAG deaktivieren | eFuse | Nein | Ja, für Produktion |
| UART-Download deaktivieren | eFuse | Nein | Ja, für Manipulationsschutz |
| Service-Terminal-Auth | App-Code | Ja | Erforderlich, wenn Terminal in Produktion exponiert ist |

**eFuse-Bits nur nach Tests im Development-Modus brennen. eFuse-Brennungen im Release-Modus sind dauerhaft und unumkehrbar.**

---

## Secure Boot v2

Verifiziert jede Stufe der Boot-Kette (Bootloader → App) mittels RSA-PSS- oder ECDSA-Signaturen.

### Signierschlüssel erzeugen
```bash
espsecure.py generate_signing_key --version 2 --scheme rsa3072 secure_boot_signing_key.pem
# Keep secure_boot_signing_key.pem offline and in a secrets manager. Never commit it.
```

### sdkconfig-Einstellungen (Development — Key wird über idf.py gebrannt)
```
CONFIG_SECURE_BOOT=y
CONFIG_SECURE_BOOT_V2_ENABLED=y
CONFIG_SECURE_BOOT_SIGNING_KEY="secure_boot_signing_key.pem"
CONFIG_SECURE_BOOT_BUILD_SIGNED_BINARIES=y
# Development: allows reflashing
CONFIG_SECURE_BOOT_ALLOW_ROM_BASIC=y   # disable for production
CONFIG_SECURE_BOOT_ALLOW_JTAG=y        # disable for production
```

### sdkconfig-Einstellungen (Produktion)
```
CONFIG_SECURE_BOOT=y
CONFIG_SECURE_BOOT_V2_ENABLED=y
CONFIG_SECURE_BOOT_SIGNING_KEY="secure_boot_signing_key.pem"
CONFIG_SECURE_BOOT_BUILD_SIGNED_BINARIES=y
CONFIG_SECURE_BOOTLOADER_NO_REBOOT_ON_FAILURE=y  # brick if verification fails
# CONFIG_SECURE_BOOT_ALLOW_ROM_BASIC is NOT set
# CONFIG_SECURE_BOOT_ALLOW_JTAG is NOT set
```

### Erstes Flashen mit Secure Boot
```bash
idf.py build
# Bootloader is signed automatically at build time with the configured key.
idf.py -p /dev/ttyUSB0 flash   # Burns Secure Boot eFuse on first successful boot
```

### OTA-Images signieren
OTA-Images müssen mit demselben Schlüssel signiert werden, der für den Bootloader genutzt wird:
```bash
espsecure.py sign_data --version 2 --keyfile secure_boot_signing_key.pem \
    --output firmware_signed.bin build/firmware.bin
```

---

## Flash-Verschlüsselung

Verschlüsselt den gesamten Flash-Inhalt (Bootloader, App, NVS, OTA-Partitionen) mit AES-XTS-256.

### Development-Modus (über Neu-Flashen umkehrbar)
```
CONFIG_FLASH_ENCRYPTION_ENABLED=y
CONFIG_FLASH_ENCRYPTION_MODE_DEVELOPMENT=y
```
- Der ESP32 erzeugt einen zufälligen Schlüssel und brennt ihn beim ersten verschlüsselten Boot in den eFuse.
- Im Development-Modus kann weiterhin per `idf.py encrypted-flash` neu geflasht werden.
- Die Klartext-Binärdatei wird vor dem Schreiben verschlüsselt.

### Release-Modus (dauerhaft — nur Produktion)
```
CONFIG_FLASH_ENCRYPTION_ENABLED=y
CONFIG_FLASH_ENCRYPTION_MODE_RELEASE=y
```
- Deaktiviert den UART-Download-Modus dauerhaft.
- Nach dem Brennen dieses eFuse ist kein Klartext-Neuflashen mehr möglich.

### Workflow für verschlüsselten Build + Flash
```bash
idf.py build
idf.py -p /dev/ttyUSB0 encrypted-flash  # initial flash (pre-encryption)
# After first boot, flash is encrypted; subsequent OTA goes through esp_ota_ops normally
```

### Binärdateien für die Werksfertigung vorab verschlüsseln
```bash
# Get the device's flash encryption key (already burned to eFuse in development mode):
espefuse.py -p /dev/ttyUSB0 burn_key BLOCK_KEY0 flash_encryption_key.bin FLASH_ENCRYPTION

# Encrypt a binary offline (for factory programming without serial access):
espsecure.py encrypt_flash_data --aes-xts --keyfile flash_encryption_key.bin \
    --address 0x10000 --output app_encrypted.bin build/app.bin
```

---

## NVS-Verschlüsselung

Verschlüsselt den Inhalt der NVS-Partition mit AES-XTS. Schützt Zugangsdaten, Kalibrierdaten und in NVS gespeicherte Geheimnisse.

### NVS-Verschlüsselungs-Key-Partition erzeugen
```bash
python $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py \
    generate-key --keytype XTS_AES_256 --key_protect_hmac \
    --kp_hmac_keygen --kp_hmac_keyfile hmac_key.bin \
    --kp_hmac_inputkey nvs_key_partition.bin
```

### sdkconfig-Einstellungen
```
CONFIG_NVS_ENCRYPTION=y
CONFIG_NVS_SEC_KEY_PROTECTION_SCHEME_HMAC=y  # or _FLASH_ENC if using flash encryption
```

### NVS zur Laufzeit mit Verschlüsselung initialisieren
```c
#include "nvs_flash.h"
#include "nvs_sec_provider.h"

nvs_sec_cfg_t nvs_sec_cfg;
nvs_sec_scheme_t *sec_scheme_handle = NULL;

// Register the HMAC-based security scheme
ESP_ERROR_CHECK(nvs_sec_provider_register_hmac(&nvs_sec_cfg, &sec_scheme_handle));

// Init NVS with the encryption scheme
esp_err_t err = nvs_flash_init_with_sec_cfg(&nvs_sec_cfg);
if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init_with_sec_cfg(&nvs_sec_cfg);
}
ESP_ERROR_CHECK(err);
```

---

## Debug-Schnittstellen deaktivieren

### JTAG (über eFuse)
```bash
# Check current JTAG eFuse state first:
espefuse.py -p /dev/ttyUSB0 summary

# Permanently disable JTAG (irreversible):
espefuse.py -p /dev/ttyUSB0 burn_efuse JTAG_DISABLE
```

Oder über sdkconfig (wird beim ersten Boot mit Secure-Boot-Release-Modus automatisch gebrannt):
```
# CONFIG_SECURE_BOOT_ALLOW_JTAG is not set
```

### UART-Download-Modus
```
CONFIG_SECURE_BOOT_ALLOW_ROM_BASIC=n  # prevents ROM serial downloader in secure boot
# For full disable (Release flash encryption also disables this):
CONFIG_ESP_CONSOLE_UART_NONE=y        # removes console UART entirely (extreme hardening)
```

---

## Härtung des Service-Terminals

Das geräteseitige Service-Terminal (siehe `references/device-terminal-console.md`) muss in Produktions-Builds kontrolliert werden.

### Entfernung zur Compile-Zeit
```c
// In app_console_commands.c or main.c:
#ifdef CONFIG_APP_SERVICE_TERMINAL_ENABLE
    app_console_init();
#endif
```
```
# sdkconfig.defaults for production:
# CONFIG_APP_SERVICE_TERMINAL_ENABLE is not set
```

### Laufzeit-Authentifizierung (falls das Terminal in Produktion bestehen bleiben muss)
```c
static bool terminal_authenticated = false;

static int cmd_auth(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: auth <token>\n");
        return 1;
    }
    // Use constant-time comparison to avoid timing attacks
    const char *expected = config_get_terminal_token();  // from encrypted NVS
    if (expected && strlen(argv[1]) == strlen(expected) &&
        memcmp(argv[1], expected, strlen(expected)) == 0) {
        terminal_authenticated = true;
        printf("Authenticated.\n");
        return 0;
    }
    printf("Authentication failed.\n");
    vTaskDelay(pdMS_TO_TICKS(2000));  // rate-limit brute force
    return 1;
}

// Guard all sensitive commands:
static int cmd_settings(int argc, char **argv)
{
    if (!terminal_authenticated) {
        printf("Not authenticated. Run: auth <token>\n");
        return 1;
    }
    // ... settings logic
}
```

---

## Sichere Programmierpraktiken

### Stack-Canaries
```
CONFIG_COMPILER_STACK_CHECK_MODE_NORM=y   # adds __stack_chk_guard checks
# or stronger:
CONFIG_COMPILER_STACK_CHECK_MODE_STRONG=y
```

### Heap-Integritätsprüfungen (Development-/QA-Builds)
```
CONFIG_HEAP_POISONING_COMPREHENSIVE=y  # expensive, use for test builds only
CONFIG_HEAP_TASK_TRACKING=y
```

### Assert-Verhalten
```
# Development: abort on assert failure (captures stack trace)
CONFIG_COMPILER_OPTIMIZATION_ASSERTION_LEVEL=2

# Production: log + reset (avoids exposing stack trace externally)
CONFIG_COMPILER_OPTIMIZATION_ASSERTION_LEVEL=1
```

### TLS-Zertifikats-Pinning für HTTPS-OTA
```c
// Embed server certificate in the firmware binary:
// In CMakeLists.txt:
//   target_add_binary_data(${COMPONENT_LIB} "server_cert.pem" TEXT)

extern const uint8_t server_cert_pem_start[] asm("_binary_server_cert_pem_start");
extern const uint8_t server_cert_pem_end[]   asm("_binary_server_cert_pem_end");

esp_http_client_config_t cfg = {
    .url = OTA_URL,
    .cert_pem = (const char *)server_cert_pem_start,
    // .use_global_ca_store = false,  // do not use — pin to specific cert
    .skip_cert_common_name_check = false,
};
```

### Lebensdauer sensibler Daten
- Geheimnisse nach Gebrauch im RAM auf null setzen: `explicit_bzero(buf, len)` oder `memset` + Compiler-Barriere.
- Zugangsdaten, Tokens oder Schlüsselmaterial auf keinem Log-Level loggen.
- Geheimnisse mit aktivierter Verschlüsselung in NVS speichern — niemals in SPIFFS oder unverschlüsseltem NVS.

---

## Checkliste für Produktions-Builds

- [ ] Secure Boot v2 aktiviert; Signierschlüssel offline gespeichert (nicht im Repo)
- [ ] Flash-Verschlüsselung im Release-Modus (oder Development-Modus für Engineering-Builds)
- [ ] NVS-Verschlüsselung für alle Geheimnis-/Zugangsdaten-Namespaces aktiviert
- [ ] JTAG per eFuse oder Secure-Boot-Release-Policy deaktiviert
- [ ] UART-Download-Modus deaktiviert (Release-Flash-Verschlüsselung) oder ROM Basic deaktiviert
- [ ] Service-Terminal entfernt oder hinter einer Zugangsdaten-Prüfung aus verschlüsseltem NVS abgesichert
- [ ] OTA-Images vor dem Ausliefern mit Secure-Boot-Key signiert
- [ ] Anti-Rollback-Zähler gesetzt und `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`
- [ ] TLS-Zertifikat für alle HTTPS-Verbindungen gepinnt (OTA, Cloud usw.)
- [ ] Stack-Canaries aktiviert (`CONFIG_COMPILER_STACK_CHECK_MODE_NORM`)
- [ ] Keine Debug-Symbole oder ausführlichen Logs im Release-Build (`CONFIG_LOG_DEFAULT_LEVEL_WARN` oder höher)
- [ ] Kein `CONFIG_OTA_ALLOW_HTTP=y` in der Produktions-sdkconfig
- [ ] `espefuse.py summary` vor der Auslieferung ausgeführt und geprüft; keine unerwarteten eFuse-Bits gesetzt
