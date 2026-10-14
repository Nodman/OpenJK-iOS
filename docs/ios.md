# OpenJK on iOS / iPadOS

## What it is

- OpenJK SP runs Jedi Academy single player. OpenJK MP is the multiplayer client.
- Both apps use rd-vanilla through gl4es on Apple's OpenGL ES path.
- Game files are not included. Copy them from your own retail game.
- OpenJK is not affiliated with Disney, Lucasfilm or Raven Software. You need your own copy of the game.

## Devices

- iOS / iPadOS 17.0 or later. A12 or newer. Arm64 only.
- iPhone XR / XS and later, including SE 2 / 3.
- iPad 8th generation or later, Air 3 or later, mini 5 or later, and iPad Pro models from 2018 or later.
- A11 and older, including A10 / A10X iPads, are outside the supported device list.
- ProMotion screens: iPhone 13 Pro through 17 Pro Max, iPhone 17, iPhone Air, and iPad Pro models from 2018 or later. Screen refresh support does not guarantee the game's frame rate.
- Fresh configs default to 120 Hz on panels that support it and 60 Hz on other panels. Video → `120 Hz (ProMotion)` offers Off / On and appears only on 120 Hz panels. Apply restarts the video. iPhone on the GL path still shows 60 Hz until the P10 Metal path. On the measured iPhone 17 Pro Max with iOS 27.0.1, `com_maxfps 120` is accepted but shows 60 Hz.
- ProMotion iPads can show 120 Hz. Measured on the 2018 iPad Pro with iPadOS 26.5 and on the M4 iPad Pro with iPadOS 27.0.1 (120.0 frames per second on the display). The 60 Hz limit is an iPhone policy on the OpenGL ES layer, not an iOS version issue.
- The 2018 A12X iPad Pro passed a 20-minute MP run at 60 Hz and auto scale. Its median window p95 frame interval was 16.72 ms.
- Memory was tested on a 4 GB iPad with `r_picmip 1` and `r_texturebits 32`. The MP map `mp/ctf1` peaked at 277 MB; the SP map `taspir2` peaked at 394 MB and loaded in 2.7 s. A real 3 GB device was not tested.

## Install

- Install SideStore or AltStore. Add the [OpenJK source](https://raw.githubusercontent.com/Nodman/OpenJK-iOS/ios/altstore/source.json), then install OpenJK SP, OpenJK MP, or both.
- With Sideloadly, download the app's IPA from [GitHub Releases](https://github.com/Nodman/OpenJK-iOS/releases). Open it in Sideloadly and sign it with your Apple ID.
- A free Apple ID needs re-signing every 7 days and allows 3 active apps. SideStore, OpenJK SP, and OpenJK MP use all three slots.
- Each release includes `SHA256SUMS` for the IPAs.

## Copy game files

- Launch each app once to create its `base` folder.
- From your Jedi Academy installation's `GameData/base`, copy `assets0.pk3`, `assets1.pk3`, `assets2.pk3`, and `assets3.pk3`.
- In Files, open **On My iPhone / iPad → OpenJK SP → base** or **OpenJK MP → base**. Put the files directly in `base`.
- SP and MP each need their own copy. Their app folders are separate.
- Return to the app and tap **Check again** in the **Game files needed** box.
- Startup checks for `assets0.pk3` and `default.cfg` for SP or `mpdefault.cfg` for MP. The cfg can be inside a pk3. The retail files supply it; `assets0.pk3` alone is not enough.
- Game pk3 files are excluded from backup. Keep your original copies.
- Put your autoexec in the same app's `base` folder: `autoexec.cfg` for MP, `autoexec_sp.cfg` for SP.

## Build yourself

- Use macOS, Xcode 26.x, and CMake 3.28 or later.
- From a checkout of the `ios` branch, run `scripts/build-ios.sh`.
- The script uses the `ios` configure preset and `ios-release` build preset. It builds both apps in Release mode.
- Without a team ID, it makes unsigned IPAs for a sideloader to sign.
- For a signed build, run `OPENJK_IOS_TEAM_ID=<your team> scripts/build-ios.sh`. Replace `<your team>` with your Xcode team ID.
- Apps: `build-ios/apps/Release-iphoneos/OpenJK-SP.app` and `OpenJK-MP.app`.
- IPAs: `build-ios/dist/OpenJK-SP-<version>.ipa` and `OpenJK-MP-<version>.ipa`. Checksums: `build-ios/dist/SHA256SUMS`.
- `OPENJK_IOS_VERSION` sets the version. Its default is `0.1.0`.
- The script refuses iOS SDK 27 or later. Select an Xcode 26.x installation with `DEVELOPER_DIR` if needed.
- CI uploads the `ipas` artifact. Tags named `ios-v*` publish both IPAs and checksums to a GitHub Release.

## Controls

- Touch: use the floating stick on the left to move. Drag on the right to look. Tap or drag in menus to use the cursor.
- **MENU** sends Escape and opens the game menu.
- Tap the console button in a map or menu to open the console and keyboard. Tap it again to close both. Closing the console another way also closes its soft keyboard.
- Hold the console button for at least 500 ms, keeping your finger on the button, to toggle the soft keyboard for menu text fields or the open console; a console hold does nothing with a hardware keyboard. Leaving the button cancels the gesture. All touch controls hide during full-screen cinematics and SP in-game videos. Tap anywhere to skip a video through the normal Escape path.
- Pad sends TAB: datapad in SP, held scoreboard in MP. Chat sends Y in MP. Both use the game's defaults and your key rebinds.
- Touch button binds:

| Label | Key | Default bind | Action |
| --- | --- | --- | --- |
| ATK | `AUX16` | `+attack` | Attack |
| ALT | `AUX17` | `+altattack` | Alternate attack |
| JUMP | `AUX18` | `+moveup` | Jump |
| DUCK | `AUX19` | `+movedown` | Crouch |
| USE | `AUX20` | `+use` | Use |
| FORCE | `AUX21` | `+useforce` | Use selected Force power |
| F>> | `AUX22` | `forcenext` | Next Force power |
| STYLE | `AUX23` | `saberAttackCycle` | Change saber style |
| <W | `AUX24` | `weapprev` | Previous weapon |
| W> | `AUX25` | `weapnext` | Next weapon |
| SABER | `AUX26` | `weapon 1` | Select saber slot |
| PAD | TAB | `datapad` (SP), `+scores` (MP) | Datapad / held scoreboard |
| CHAT | Y | `messagemode` (MP) | Chat |

- Gamepad: left stick moves; right stick looks. In menus, left stick moves the cursor, A / Cross selects, and B / Circle goes back.
- SDL button `n` maps to `A_JOY0+n`. Console bind names are `JOY0`, `JOY1`, and so on. Start sends `A_ESCAPE`. LT sends `A_JOY21`; RT sends `A_JOY22`.
- Default gamepad binds. Face labels use Xbox / PlayStation names:

| Button | Key | Default bind | Action |
| --- | --- | --- | --- |
| A / Cross | `JOY0` | `+moveup` | Jump |
| B / Circle | `JOY1` | `+movedown` | Crouch |
| X / Square | `JOY2` | `+use` | Use |
| Y / Triangle | `JOY3` | `+useforce` | Use selected Force power |
| Back / Share | `JOY4` | `+scores` | Scores |
| Left stick press | `JOY7` | `+speed` | Walk / run |
| LB / L1 | `JOY9` | `weapprev` | Previous weapon |
| RB / R1 | `JOY10` | `weapnext` | Next weapon |
| D-pad up | `JOY11` | `forcenext` | Next Force power |
| D-pad down | `JOY12` | `forceprev` | Previous Force power |
| D-pad left | `JOY13` | `saberAttackCycle` | Change saber style |
| LT / L2 | `JOY21` | `+altattack` | Alternate attack |
| RT / R2 | `JOY22` | `+attack` | Attack |
| Start / Options | Escape | — | Menu / back |

- Keep `in_joystick 1` for the touch stick and pad axes. Keep `j_twinStick 1` for pad look.
- iPad keyboard and mouse use the normal key and mouse binds. Text input stays active for the console unless the console button hid its soft keyboard, MP chat, menus with a hardware keyboard, and menus where the console button requested the soft keyboard. It is off in gameplay and cinematics. Keyboard disconnect hands active text entry to the soft keyboard. Input and video restarts restore this state.
- Rebind in the console. For example, `bind AUX16 +altattack` changes ATK. Use the same command with a `JOY` key to change a pad bind.
- Real Xbox / DualSense, Smart Keyboard, and mouse checks remain release checks. The recorded touch checks used synthetic input.

## Settings

- Dynamic glow is unavailable on the GL path, so its More Video row is hidden. Light flares can be enabled in More Video and persist. Flares use a depth read-back each frame and may cost frame time on tile GPUs.

- Enter commands in the console, or put `seta` lines in that app's autoexec (`base/autoexec.cfg` for MP, `base/autoexec_sp.cfg` for SP).
- Video → Resolution offers Auto, Native, 3/4 and Native/2 with the predicted pixel size. Native is the default. Apply restarts the video. For custom menus, use `seta r_renderScale 0|1|0.75|0.5` (choose one value), then `vid_restart`. `r_mode` stays a desktop selector.
- `r_renderScale 1`: default Native resolution. `r_renderScale 0` selects Auto. A value from `0.5` to `1.0` sets a fixed scale. Lower values reduce detail and GPU load. Run `vid_restart` to apply a change.
- Auto scale: below Apple GPU family 7 uses `0.6`; families 7 / 8 use `0.75`; family 9 or later uses `1.0`. With `com_maxfps` above 60, auto scale is multiplied by `0.8`. Scale is clamped to `0.5`–`1.0`, then raised for a drawable at least 540 px tall, up to full scale.
- Video → `120 Hz (ProMotion)` stages `com_maxfps` 60 / 120 on 120 Hz panels. On is the default on these panels; other panels default to 60. Auto resolution previews the staged choice. iPhone on GL still shows 60 Hz until P10. Console fallback: `seta com_maxfps 60|120` (choose one value), then `vid_restart`.
- Existing saved configs keep their resolution and FPS values. To adopt the new defaults on a 120 Hz panel, enter `seta r_renderScale 1` and `seta com_maxfps 120`, then `vid_restart`.
- Video → `FPS counter` offers Off / On. Apply shows a small gold frame-rate readout at the top right inside the safe area in a map. Off is the default.
- `in_touchControls 0`: hide touch controls except the console button. `1`: default; hide them when a pad is connected, or a hardware keyboard and mouse/trackpad are both connected. A keyboard alone keeps touch look and buttons. `2`: keep them visible with these devices. Menus and the console show only the console button. Full-screen cinematics and SP in-game videos hide all controls in every mode; camera cutscenes still show them.
- `in_touchLookScale 1.0`: default touch look scale. Higher values turn faster.
- `j_lookSpeed 1.5`: default pad look scale. Higher values turn faster.
- `j_deadzone 0.15`: default stick dead zone. Raise it if a stick drifts.
- `j_lookExponent 2.0`: default right-stick response curve.
- `r_texturebits 32`: device default. `r_picmip 1` is set on devices with RAM at or below the device code's 3500 MiB threshold. `r_surfaceSprites 0` is set on A12 / A13 class GPUs.
- `r_surfaceSprites` and `r_texturebits` use `CVAR_ARCHIVE_ND`. A value equal to the engine default is not saved. For example, `seta r_surfaceSprites 1` on A12 is lost at restart. Put such overrides in the app's autoexec file.
- `r_gl4esEnv` defaults to `LIBGL_USEVBO=3`. gl4es reads it once at renderer init. Changes after init are ignored, including across `vid_restart`. Fully relaunch with the setting supplied at startup, for example `+set r_gl4esEnv LIBGL_USEVBO=3`.
- `ios_frameStats 1` logs frame intervals, frame and present times, and memory use. `ios_frameStats 0` stops the log. `ios_frameStatsReset` starts a new measurement window.
- `cg_drawFPS 1` shows the engine frame rate in a map. For frames the display actually showed, plug the device in by USB and run `scripts/ios/shown-fps.py --device <name, OS version or id>`. It records a 3 second Metal System Trace and counts the app's displayed frames. Apple's Metal Performance HUD does not draw over the GL layer.
- Frame-stats CPU counters measure wall time. They can include driver waits. They are not pure CPU work counters.

### Aspect-correct 2D, FOV and safe area

- On screens wider than 1.5:1, the 640×480 menus, HUD and console keep their 4:3 shape. They use one scale in both directions and sit in the center with side gutters. The 3D view still fills the screen. iPads with a ratio below 1.5 keep the old stretched 2D.
- The following cvars are archived. Saved values and autoexec settings win over defaults.
- `r_aspectCorrect 1`: iOS default. Enables aspect-correct 2D. The desktop default is `0`.
- `r_aspectCorrectMinRatio 1.5`: default minimum screen ratio for aspect-correct 2D.
- `r_aspectStretchBackdrops 0`: iOS default. Loading art and menu-owned backgrounds fit 4:3 with black gutters. `1` stretches that art over the full screen. Retail main menus build their art from items, so they always stay 4:3.
- `cg_fovAspectAdjust 1`: iOS default. Keeps the 4:3 vertical field of view on wide screens. `0` restores the old crop. `cg_fov` stays at its default of `80`.
- Use `seta r_aspectCorrect 0` to restore the old 2D path for mods or menus that already compensate for stretch. FOV has its own switch, `cg_fovAspectAdjust`.
- Console text, chat and the touch overlay keep clear of the camera cutout and home indicator in both landscape orientations. `r_aspectSafeLeft`, `r_aspectSafeTop`, `r_aspectSafeRight` and `r_aspectSafeBottom` are read-only values in drawable pixels for reference.
- iPads below the 1.5 band get no new safe margins for 2D. The Face ID iPad's roughly 20 pt bottom inset is unchanged behavior, not a bug.
- The HUD stays centered. Side gutters stay free for touch controls. An “edges” option is planned for a later version.
- Taps and the touch overlay follow the same mapping. Nothing needs configuring.
- JK2 mode is not built for iOS. A JK2 build on a wide desktop with `r_aspectCorrect 1` is unsupported. Third-party menus or HUDs that already compensate for stretch may look wrong; use `seta r_aspectCorrect 0`.

### Lifecycle

- Backgrounding pauses frames and sound, releases held input, and saves the config. Returning resumes the app. MP disconnects from a remote server if time away exceeds `cl_timeout`.
- Audio uses SoloAmbient. It respects the silent switch and stops other audio.
- The screen does not sleep while the app runs.

## Troubleshooting

- **Game files needed:** check the app's own `base` folder and copy all four retail pk3 files. Tap **Check again**. A cfg inside a retail pk3 does not need to be extracted.
- **Stutter:** lower `r_renderScale`, then run `vid_restart`. Use `com_maxfps 60`.
- **Shadows:** keep `cg_shadows 1`. Stencil shadows with `cg_shadows 2` read back the full screen every frame. Keep `r_measureOverdraw 0`; its stencil readbacks are not supported on GLES2.
- **Refraction:** keep `cg_renderToTextureFX 0` and `cg_saberTrail 1`. `cg_renderToTextureFX 1` and `cg_saberTrail 2` enable refraction. It copies the screen mid-frame and costs frames on mobile GPUs.
- **Pure server rejects a join:** extra pk3 files in `base` can block `sv_pure` joins. Check that folder for added files.
- **Hot device:** the message is “Device is very hot: capped at 30 fps. Lower r_renderScale to help.” The thermal cap changes the frame rate. It does not lower scale for you.
- **Crash on a 3 GB device during load:** put `seta r_texturebits 16` in the app's autoexec (`base/autoexec.cfg` for MP, `base/autoexec_sp.cfg` for SP), then relaunch. Real 3 GB devices were not tested.
- **LAN connection:** allow the Local Network prompt on first LAN use. LAN browsing is not supported; connect by server address.
- **Custom client mods:** servers that need custom native client modules do not work. The apps load their embedded modules.
- **No sound:** check the silent switch. Real listening and interruption checks on both test devices remain release checks.
- **Disconnected after background:** time away longer than `cl_timeout` disconnects a remote MP session. Rejoin the server.
