# Local AI Service (replacing Gemini)

This document describes the planned replacement of the cloud Gemini
integration with a fully local backend running on the household server
("Kraken", a Raspberry Pi 5). It supersedes `docs/gemini-service.md` for the
`folloup-waveshare` line of this fork -- the device has no reason to talk to
Google once a local model does the same job on the same LAN.

**Status: code written and committed, nothing built, nothing deployed.** The
firmware and frontend changes described below have been made in full on the
`local-ai-plan` branch. This fork has no ESP-IDF toolchain and no
node/npm installed in the environment this was written in, so **none of it has
compiled or type-checked, let alone run on hardware**, and nothing on Kraken
was changed to serve any of this. See "Umsetzungsstand" at the end of this
document for exactly what is done, what is written-but-unverified, and what is
still missing.

## Why replace Gemini at all

Owner decision: the Waveshare ESP32-S3 board (ordered, not yet in hand) should
not depend on a cloud account, a stored API key, or Google's availability.
Kraken already runs a local LLM (LM Studio, OpenAI-compatible REST API) and a
local ASR pipeline (faster-whisper, via `imkopfhaben-brain`) for an unrelated
project. Reuse both rather than building anything new.

## What was actually verified (2026-09-01, on Kraken)

- LM Studio's local server exposes an OpenAI-compatible
  `POST /v1/chat/completions`. Confirmed working against `google/gemma-4-e2b`
  (currently loaded, 4.6B, `gemma4` arch -- this is LM Studio's listing name
  for a Gemma-3n-class model).
- **Audio input is not accepted by the server**, despite the model itself
  being nominally multimodal (an `mmproj` file is loaded). Tried both
  OpenAI's `input_audio` content-part shape and an `audio_url` shape; both
  were rejected with the same error: `'content' objects must have a 'type'
  field that is either 'text' or 'image_url'`. **Do not build a transcription
  path through gemma/LM Studio.** Transcription stays on faster-whisper.
- **`gemma-4-e2b` is a reasoning model and reasoning is expensive.** A
  one-sentence trivial prompt used 122 completion tokens by default -- 114 of
  them were hidden `reasoning_content`, not the actual answer. Passing
  `"reasoning_effort": "none"` in the request suppressed reasoning entirely
  (0 reasoning tokens, correct answer, much faster). **This field is
  mandatory in every request**, not an optimization -- without it, prompts
  will blow the local context window on reasoning tokens before producing
  real output.
- **There is no token-counting endpoint.** `POST /v1/internal/tokenize`
  returns `"Unexpected endpoint or method"`. `summary_service`'s existing
  `gemini_service::CountTokens()` call has no local equivalent and must be
  dropped in favor of the character-based estimate the code already has as a
  fallback (`EstimateTokenCount()`, `text.size() / 4`) -- just use it
  unconditionally instead of calling out to the provider first.
- The response text lives at `choices[0].message.content`, separate from
  `reasoning_content` -- no `<think>` tag stripping needed, unlike some other
  local reasoning models.
- LM Studio's server was bound to `127.0.0.1` only, unreachable from the LAN
  (and therefore from the ESP32 board). Rebound to `0.0.0.0` on owner
  confirmation (2026-09-01) and confirmed reachable at
  `http://192.168.178.215:1234/v1/models` from outside localhost.
  **Not yet durable**: it was restarted manually (`lms server start --port
  1234 --bind 0.0.0.0`), there is no systemd unit, and behavior after a real
  reboot has not been tested. Deliberately not built blind -- see Open
  follow-ups.
- Kraken's LAN IP at time of writing: `192.168.178.215`. This will need to be
  either hardcoded as a Kconfig default or, better, resolved by mDNS
  (`kraken.local`) if the ESP32 mDNS stack can resolve it reliably on the
  same network the board's Wi-Fi service already joins.
- `imkopfhaben-brain/ai_service.py` already has a clean, standalone
  `transcribe_audio(audio_path: str) -> str` using `faster_whisper.WhisperModel`
  (already loaded, already proven in production for the imkopfhaben project).
  It is decoupled from imkopfhaben's note/category pipeline -- safe to call
  from a new, unrelated route.

## Target architecture

```
ESP32-S3 board (folloup-waveshare)
  |
  |-- transcription  -->  POST http://kraken:PORT/api/transcribe-raw   (new, thin wrapper)
  |                         multipart or raw WAV body -> {"transcript": "..."}
  |                         backed by imkopfhaben-brain's ai_service.transcribe_audio()
  |
  |-- summaries/text -->  POST http://192.168.178.215:1234/v1/chat/completions
  |                         model=google/gemma-4-e2b, reasoning_effort=none
  |
  \-- readiness check -> GET  http://192.168.178.215:1234/v1/models
                            (replaces the old Gemini model-GET auth probe;
                            no credential needed)
```

No API key, no NVS-stored secret, no TLS/CRT bundle needed for the LLM or
transcription calls (both are plain HTTP on the LAN). `esp_crt_bundle_attach`
usage in `gemini_service.cpp` becomes dead code for these paths.

## Firmware changes (written, unverified -- see Umsetzungsstand)

All three call sites funnel through `components/gemini_service/`, so the
blast radius is contained -- `transcription_service`, `summary_service`,
`app_shell`, `status_bar_runtime`, `epaper_ui` (status star) and
`feedback_service` (connect-cue) all consume `gemini_service::Snapshot` /
`GenerateText` / `Transcribe` / `CountTokens` and do not need to change if
the public API shape is kept and only re-implemented against the local
backend internally. Recommend keeping the component's public surface stable
and swapping internals, to avoid a repo-wide rename with no way to compile
and check it here.

1. **`kGeminiApiBaseUrl`** -> Kconfig-configurable local base URL
   (`CONFIG_FOLLOWUP_LOCAL_AI_BASE_URL`, default
   `http://192.168.178.215:1234/v1/`), no `x-goog-api-key` header.
2. **`BeginAuthentication()` / readiness probe** -> `GET {base}models`
   (`/v1/models`), 200 = ready. No key precedence logic needed; "configured"
   can just mean "have a base URL," which is always true once a sane
   default ships.
3. **`GenerateText()`** -> `POST {base}chat/completions` with
   `{"model":"google/gemma-4-e2b","messages":[{"role":"user","content":prompt}],"temperature":0,"reasoning_effort":"none"}`;
   parse `choices[0].message.content` instead of
   `candidates[0].content.parts[].text`.
4. **`CountTokens()`** -> delete the HTTP round-trip; callers
   (`summary_service::CountPromptTokens`) should call the existing
   `EstimateTokenCount()` fallback unconditionally.
5. **`Transcribe()`** -> replace the Gemini resumable-upload dance
   (`PerformUploadStart` / `PerformUploadFinalizePcmWav` / `generateContent`
   with `fileData`) with a single `POST` of the same WAV body (built with
   the existing `BuildWavHeaderPcm16Mono` + `RecordedClip::ForEachChunk`) to
   the new `/api/transcribe-raw` endpoint; parse `{"transcript": "..."}`.
6. **Token budgets in `summary_service.cpp` must shrink drastically.** They
   were sized for Gemini's huge context window:
   `kSummaryInputTokenBudget=120000`, `kSummaryChunkTokenBudget=60000`,
   `kSummaryRollupTokenBudget=120000`. The local server runs with
   `--ctx-size 8192`. These need to come down to roughly
   `kSummaryInputTokenBudget≈3000`, `kSummaryChunkTokenBudget≈2500`,
   `kSummaryRollupTokenBudget≈3000` (leaving headroom in the 8192 window for
   the prompt scaffolding and the response) -- exact numbers should be tuned
   against real measured prompt sizes once the board exists, not guessed
   further than this.
7. **Settings/API-key UI** (`epaper_ui/settings_page.cpp`, the portal
   `/api/settings/gemini*` routes, and the `webserver/src` frontend) still
   assume a stored secret. Out of scope for this pass -- flagged below.

## Open follow-ups (deliberately not done yet)

- No systemd unit for LM Studio's server. It needs to survive a Kraken
  reboot for the board to have a reliable target; this was not built blind
  and needs a real reboot test, which was skipped to avoid disrupting the
  live machine mid-session.
- No `/api/transcribe-raw` endpoint exists yet on Kraken -- needs to be added
  (a few lines reusing `ai_service.transcribe_audio`), and exposed either as
  a new route on `imkopfhaben-brain` or a tiny standalone service. Not built
  yet because the exact hosting choice should follow from how the owner
  wants Kraken's growing pile of small local services organized, not decided
  unilaterally here.
- The C++ changes above are unwritten and unverified -- this fork's build
  environment has no ESP-IDF toolchain installed, so nothing here has
  compiled even once.
- Settings-page / portal / frontend rework for a keyless provider (item 7
  above) is unscoped.
- `kraken.local` mDNS vs. hardcoded LAN IP for the base URL is an open
  choice -- the IP is simpler and was what was actually tested; mDNS is more
  robust to Kraken's IP changing but adds a resolution step on the ESP32
  side that hasn't been checked against this project's Wi-Fi/mDNS stack.

## Umsetzungsstand (2026-09-02, third pass)

**Compiled and verified this pass:** ESP-IDF v5.4 + npm were installed on
Kraken specifically to check this. `idf.py build` (target esp32s3) now runs
to completion -- `folloup_sticky.bin` is generated, 65% app-partition space
free. `npm run build` in `webserver/` (tsc -b + vite build) also succeeds
with zero errors, and the built bundle was copied into
`components/wifi_service/portal/` (see "Done" below -- this closes the
"deliberately left untouched" gap from the previous pass). Two pre-existing
compile errors, unrelated to the local-AI changes and present in the
`folloup-waveshare` branch before any of this work started, were found and
fixed along the way -- see "Fixed this pass" below. This is the first time
this branch has compiled successfully at all.

Still true: nothing on Kraken beyond the LAN-bind of LM Studio's server and
the (currently syntax-checked-only, not live-tested) `/api/transcribe-raw`
addition to `imkopfhaben-brain/main.py` was touched or deployed. No systemd
units installed. No real hardware exists yet to flash this onto or to verify
runtime behavior (network calls, audio, display, power) -- a clean compile
proves the code is structurally sound, not that it works on the device.

### Done (written, matches the plan above)

- `components/gemini_service/` renamed to `components/local_ai_service/`
  (directory, files, namespace, CMakeLists `REQUIRES`). Public API shape kept
  close to the original so callers didn't need structural rework, just
  renamed calls.
- Readiness probe, `GenerateText`, and `Transcribe` re-implemented against
  the local backend as specced (base URL + `/models`, `/chat/completions`
  with `reasoning_effort: "none"`, single-POST WAV upload to a transcription
  URL).
- `CountTokens` / `TokenCountResult` removed entirely; `summary_service`'s
  `CountPromptTokens` now calls the existing character estimate directly,
  no remote call.
- `summary_service` token budgets shrunk to fit an 8192-token local context
  (3000/2500/3000, down from 120000/60000/120000) -- these are a starting
  estimate, explicitly not tuned against real prompt sizes.
- Transcription readiness (in both `transcription_service` and
  `recording_session_service`) is now gated on "is a transcribe URL
  configured", not on the chat model's readiness -- catches a bug in my own
  first pass where I'd copied the old single-provider gate and conflated two
  independent local services.
- Every `gemini`/`Gemini` identifier, log message, and UI string in the
  firmware C++ and the `webserver/src` TypeScript/HTML source was found
  (`rg -il gemini`, swept twice) and renamed or reworded, except the ones
  listed as deliberately untouched below.
- No new image assets. The existing star icon (`EmbeddedIconId::kStar`) is
  reused as-is for the status-bar/lock-screen "AI ready" indicator, same as
  before.
- Top-level `README.md` and `webserver/README.md` updated so they don't
  describe Gemini as the current backend.
- `docs/gemini-service.md` marked superseded at the top, left otherwise
  intact as a historical record (upstream and other branches of this fork
  may still be Gemini-based).

### Fixed this pass (2026-09-02): readiness/validation bugs + pre-existing build errors

- `local_ai_service.cpp` `Authenticate()`: readiness reported `success=true`
  on any HTTP 2xx from `/v1/models`, even if the configured model wasn't in
  the returned list -- a wrong/unloaded model name showed as "connected"
  with no error. Now `success = model_listed`, with a distinct status
  message ("... but configured model not loaded") instead of the generic
  "unreachable" text.
- `local_ai_service.cpp` `ApplySettingsPatch()`: `base_url` was rejected
  with 400 if it normalized to empty; `transcribe_url` had no equivalent
  check and would silently store as an empty string. Now symmetric.
- `xpowers_axp2101_driver.cc:2504` (`log_d` format-string / `uint32_t` vs
  `%x` mismatch) and `board_es8311_codec.cc` (`.bclk_div` field, removed
  from `i2s_std_clk_config_t` in this IDF version; the surrounding comment
  already said it's a no-op in master role, so dropping it is
  behavior-preserving) -- both pre-existing, both unrelated to the local-AI
  port, both blocked *any* build of this branch regardless of the AI
  changes. Fixed because getting a clean build was the point of this pass;
  see the firmware repo's git history for the exact diff.

### Deliberately left untouched

- `components/project_assets/` (`kGeminiApi` icon, from
  `assets/icons/gemini_api.png`, and `assets/epaper_assets.json`'s entry for
  it) -- this generated asset was already unreferenced by any page before
  this change (confirmed: no hits for `EmbeddedIconId::kGeminiApi` outside
  its own definition), so leaving it alone doesn't change behavior. Renaming
  or removing it would mean touching generated asset-pipeline output, which
  was out of scope ("keine neuen Assets").

### Design decisions worth a second look in review

- `providerKeys.ts`: rather than forcing the base URL into the old
  masked-secret UI pattern (`******last4`, hidden save button once a key
  exists), I wrote a small dedicated local-AI controller that shows the URL
  in plain text and always allows editing -- a URL isn't a secret, and the
  old pattern's whole point was to never show a stored secret back to the
  page. Flagging this as a deliberate deviation from "reuse the exact
  pattern," not an oversight.
- Only `base_url` is exposed in the web settings UI; `transcribe_url` is
  settable via the backend's `PATCH /api/settings/local_ai` (it accepts
  both fields) but has no UI field yet. Kconfig/NVS-default-only for now.
- Dropped several Gemini-specific `RuntimeSnapshot`/error-code fields that
  had no local equivalent rather than faking them: `has_sdkconfig_api_key`
  (there's no separate "is there a fallback" question when the base URL
  always has a Kconfig default), `api_key_last4` (nothing to mask), and the
  `missing_api_key`/`invalid_api_key` error codes (replaced with
  `missing_fields`/`invalid_base_url`, since the validation question is now
  about a URL, not a key). `supports_audio_understanding` /
  `supports_structured_output` were kept as always-`false` fields, same
  shape as before, but now with a comment pointing at the actual verified
  reason (LM Studio's API rejects audio content parts) instead of "not yet
  ported."

### Not done / needs a decision before it can be

- `/api/transcribe-raw` exists in `imkopfhaben-brain/main.py` on Kraken now
  (see `kraken-arche` PR #1's `transcribe_raw_patch.md`, applied), syntax-
  checked (`python3 -m py_compile`), but **not live-tested**. The service
  wasn't running when this was applied (its own project's convention:
  manual-start-only, no autostart), and starting it to test was blocked by
  the Auto-Mode classifier plus a since-clarified concern about accidental
  autostart -- see PR #1 / session log for details. Real end-to-end test
  (POST a WAV, check the transcript comes back) is still open.
- No systemd unit for LM Studio's server; still a manually-started process
  bound to `0.0.0.0:1234`. Draft unit exists in `kraken-arche` PR #1,
  unverified (no reboot test).
- No real hardware to flash and run this on yet (board on order). A clean
  compile is not a runtime guarantee -- network behavior, audio, e-paper
  refresh, and power management are all unverified beyond reading the code.
- **Automated review re-run 2026-09-02, completed this time** (partially --
  4 of 8 finder-agent angles didn't return in time; the coordinator finished
  from its own manual read plus the 4 angles that did return, rather than
  fabricate the missing ones). Findings:
  - **Fixed, verified against pre-diff `PerformGet`/`PerformJsonPost` for
    comparison:** `PostWavClip()` set both an `event_handler` (which
    appends every `HTTP_EVENT_ON_DATA` chunk to `response.body`) *and*
    manually drained the body again via `esp_http_client_read()` after
    fetch_headers -- ESP-IDF fires that event during manual `read()` calls
    too, not just `_perform()`, so every real transcription response came
    back double-appended (`{"transcript":"x"}{"transcript":"x"}`), which
    would have broken JSON parsing on every real recording. Fixed by
    dropping the event handler from that one function (it's the only one
    of the three HTTP call sites that reads manually instead of using
    `_perform()`).
  - **Confirmed real, not fixed -- needs a decision:** transcription
    readiness (`recording_session_service.cpp`) now gates only on "is a
    transcribe URL configured" (a static config string), not on any live
    reachability check -- unlike `base_url`/the chat model, which has an
    `Authenticate()` round-trip. An offline device or a down transcription
    server now blocks for the full `kTranscribeTimeoutMs` (30s) per attempt
    instead of failing fast. Fixing this properly means adding a health-
    check subsystem for the transcribe endpoint symmetric to
    `Authenticate()` -- scoped as a real design decision, not a one-line
    patch.
  - Re-confirmed, not new: `transcribe_url` still has no UI field (already
    listed under "Design decisions" above).
  - Lower-severity duplication/efficiency findings (not independently
    re-verified by me, taken from the completed finder angles as-is):
    repeated HTTP-client boilerplate across `PerformGet`/`PerformJsonPost`/
    `PostWavClip`; `providerKeys.ts` reimplements the busy-flag/error
    wrapper already generalized elsewhere in the same file;
    `BuildChatCompletionRequestBody` reimplements the existing
    `JsonString()` helper instead of calling it; `summary_service`'s
    input-trimming loop rebuilds/rescans the whole remaining prompt on
    every removed entry, now running far more often under the ~40x-smaller
    local token budget; `GenerateSummary()` re-fetches `base_url`/
    `model_name` from a snapshot it already holds.
