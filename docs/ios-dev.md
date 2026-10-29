# iOS developer notes

Companion to the [player guide](ios.md), the [advanced settings](ios-advanced.md) and the [ANGLE dependency note](ios-angle.md).

## Build

- Use macOS, Xcode 26.x and CMake 3.28 or later. From a checkout of the `ios` branch, run `scripts/build-ios.sh`. A clean configure downloads the pinned ANGLE package.
- Without a team ID it makes unsigned IPAs for a sideloader to sign. For a signed build: `OPENJK_IOS_TEAM_ID=<your team> scripts/build-ios.sh`.
- IPAs: `build-ios/dist/OpenJK-SP-<version>.ipa` and `OpenJK-MP-<version>.ipa`, with `SHA256SUMS`. `OPENJK_IOS_VERSION` sets the version (default `0.1.0`).
- CI uploads the `ipas` artifact on every push. Tags named `ios-v*` publish both IPAs and checksums to a GitHub Release and refresh `altstore/source.json`.
- `scripts/build-ios.sh` uses the `ios` configure preset and `ios-release` build preset. It builds both apps in Release mode.
- Apps: `build-ios/apps/Release-iphoneos/OpenJK-SP.app` and `OpenJK-MP.app`.
- The script refuses iOS SDK 27 or later (SDL2 needs the UIScene work first). Select an Xcode 26.x install with `DEVELOPER_DIR` if needed.
- `scripts/ios/check-modules.sh build-ios all` checks module exports, the native GLES boundary and the embedded ANGLE bytes. CI runs it after the build.
- The version in the console corner and the `version` cvar is `OpenJK-iOS: v<OPENJK_IOS_VERSION>` for MP and `OpenJK-iOS SP: v...` for SP. MP servers see it as `gamename`.
- Release tags `ios-v*` use `docs/ios-release-notes.md` as the GitHub Release body.

## Host tests

- `tests/ios/*_host.py` compile the real engine code against host stubs. Run each with `python3`; no device needed.
- `tests/ios/angle_consumer_host.py <assets>` needs a directory with the four pinned ANGLE assets.
- C++ unit tests: configure with `-DBuildTests=ON`, build `UnitTests`, run `build-tests/UnitTests`.
- Desktop and sanitizer checks cover touch geometry, gestures, visibility, command timing, aim rates and icon decoding with synthetic input. Touch feel, icon appearance, controllers and keyboards remain device checks.

## Measurement

- `ios_frameStats 1` logs frame intervals, frame and present times, and memory use. `ios_frameStats 0` stops it; `ios_frameStatsReset` starts a new window. CPU counters measure wall time and can include driver waits.
- `cg_drawFPS 1` shows the engine frame rate. For frames the display actually showed, plug the device in by USB and run `scripts/ios/shown-fps.py --device <name, OS version or id>`. It records a 3 s Metal System Trace and counts the app's displayed frames. Apple's Metal Performance HUD does not draw over EAGL.

## Measured results

- iPhone 17 Pro Max, iOS 27.0.1: EAGL shows about 60 fps even at `com_maxfps 120`. ANGLE showed about 117–119 displayed fps in short MP and SP runs. Sustained and thermal behavior were not qualified.
- 2018 and M4 iPad Pro: EAGL shows 120.
- 2018 A12X iPad Pro: 20-minute MP run at 60 Hz and auto scale; median window p95 frame interval 16.72 ms.
- 4 GB iPad with `r_picmip 1` and `r_texturebits 32`: `mp/ctf1` peaked at 277 MB; `taspir2` peaked at 394 MB and loaded in 2.7 s. No 3 GB device was tested.

## Defaults and internals

- Device defaults come from `Sys_IOS_DeviceDefaults()` and carry `CVAR_PLATFORM_DEFAULT`, so `reset` returns to the panel value. `com_maxfps 120` is added on 120 Hz panels.
- Auto render scale: below Apple GPU family 7 uses `0.6`; families 7 / 8 use `0.75`; family 9 or later uses `1.0`. Above 60 fps the value is multiplied by `0.8`. It is clamped to `0.5`–`1.0`, then raised until the drawable is at least 540 px tall.
- Thermal: the target is capped at the panel maximum, lowered to 60 at Serious and 30 at Critical. Scale and the saved rate are untouched.
- `r_gl4esEnv` defaults to `LIBGL_USEVBO=3`. gl4es reads it once at renderer init, so changes need a full relaunch with `+set r_gl4esEnv ...`.
- `r_aspectSafeLeft/Top/Right/Bottom` are read-only safe-area insets in drawable pixels. iPads below the 1.5 ratio get no extra 2D margins. JK2 mode is not built for iOS.
- `r_glesProvider` is `CVAR_ARCHIVE`, read once at the first video mode set; later changes wait for the next launch. `r_glesProviderActive` is `CVAR_ROM` and `seta` on it is refused.
- The Video page `Graphics backend` row is the retail `video_sync` item rebound to `r_glesProvider` with a string list, like the other iOS rows in `UI_IOS_PatchVideoMenu`.

## Touch internals

- Central Force Use and the quick slots share one touch owner. Competing touches are ignored until the owner lifts. Editing, availability loss, map changes and backgrounding cancel ownership and held powers.
- MP keeps one pending one-shot Force command; a quick one-shot stays busy until its finger lifts and the command is sent. Demo playback refuses new Force presses.
- Custom AUX21 bind text already expanded into the command buffer can still run after touch cancellation. The default `+useforce` and stock held commands get touch-release handling.
- Saved layouts from older versions migrate, keeping their on-screen centers where possible, then clamp after resize or rotation.
