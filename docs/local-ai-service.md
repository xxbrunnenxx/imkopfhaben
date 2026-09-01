# Local AI Service (replacing Gemini)

This document describes the planned replacement of the cloud Gemini
integration with a fully local backend running on the household server
("Kraken", a Raspberry Pi 5). It supersedes `docs/gemini-service.md` for the
`folloup-waveshare` line of this fork -- the device has no reason to talk to
Google once a local model does the same job on the same LAN.

**Status: plan + verified facts only. No firmware code has been changed yet.**
This fork has no ESP-IDF toolchain installed in the environment this document
was written in, so no C++ change described below has been compiled, let alone
run on hardware. Treat everything under "Firmware changes" as a spec to
implement, not a diff that was applied.

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

## Firmware changes needed (spec, not yet implemented)

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
