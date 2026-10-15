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

- Put a finger on the lower left to move, or on free space on the right to aim. The move stick appears at touchdown and stays there until you lift.
- Aim defaults to a floating joystick. A small deflection turns slowly; a larger one turns faster. Holding it keeps turning. Returning to its center or lifting stops the turn. The Aim tab also offers Free drag: move a finger to turn, and stop moving it to stop turning. Free drag has no visible aim stick.
- Attack, alternate attack and jump form an arc on the right. Attack and alternate attack press immediately; hold to keep them pressed. Drag past the small movement threshold to aim while attacking. Free drag starts measuring distance at that threshold, without a jump. Lifting ends fire and aim. Stock cooldown and charge rules still apply.
- The first aiming finger keeps aim until it lifts. Another finger may attack but cannot take aim later. Lift and press again to start a new aiming gesture. A deflected physical right stick takes priority over touch aim; suppressed drag is discarded and will not replay.
- Attack uses the supplied swing glyph for sabers and trigger glyph for guns or melee. Alternate attack uses the same supplied alternate glyph for every weapon. Unknown weapons use the saber pair.
- Use sits at the bottom left. Crouch and Style sit near the bottom right. Style uses a blue saber at 10° for fast stance, yellow at 45° for medium, and red at 90° for strong. Dual follows its blue/yellow stance; staff stays yellow even with one blade off. Unknown stance is white. Saber selection keeps its supplied diagonal angle.
- Previous weapon, Saber and Next weapon run left to right at the top right. Saber selects the saber, or toggles its blade when already selected. Force Previous / Use / Next sit below them, with four quick slots to their left. MP melee selection has no dedicated touch button.
- Menu, MP Chat and Pad sit at the top left. Menu sends Escape. Chat opens global chat and the soft keyboard. Pad opens the SP datapad, or holds the MP scoreboard until you lift.
- Menu, Chat, Pad, Console, Lock, weapon arrows and Force controls have half their former width and height, including hitboxes. Console retains half of its own former corner size. Combat buttons, Saber, Style and sticks keep their sizes. Hitboxes remain rectangular, including for round glyphs.
- Supplied thin glyph strokes are preserved. Round buttons have no filled background. Menu, Chat, Pad, Console and Lock keep rectangular backgrounds; joystick graphics keep their filled base and knob. Empty slots and round drag highlights use outlines in the editor.
- Buttons keep their positions when others hide. Saber hides when unowned. SP Style follows learned legal stances and useful blade toggles. MP Style stays visible for an owned saber while equipment actions are allowed, even at Saber Offense 1. The stock command still enforces weapon, class, stance and blade restrictions; visibility does not grant another stance.
- Weapon arrows hide with only one selectable weapon. Force arrows hide when cycling is not useful. The SP datapad remains available in a map before you learn Force powers. Chat is MP-only. Death, spectator and intermission keep basic actions and sticks while equipment controls hide.
- For older custom modules that never report control availability, basic controls return after about a second in a map. Central Force Use keeps its stock binding and fallback glyph, but quick slots need a fresh learned-power report. SP still has no Chat. Menu, cinematic and hardware-hide rules still apply.
- Tap or drag elsewhere in menus to use the cursor. Tap Console in a map or menu to open the console and keyboard; tap again to close them. Closing the console another way also closes its soft keyboard.
- Hold Console for at least 500 ms, keeping your finger on it, to toggle the soft keyboard for menu fields or the open console. A hold does nothing with a hardware keyboard. Leaving the button cancels the gesture. Full-screen cinematics and SP in-game videos hide all controls; tap anywhere to skip through the normal Escape path.

### Force selection and quick slots

- Previous / Next use the stock Force selection order. Use activates the selected power. Selected Use, quick slots and the picker share original icons loaded from your game files; missing or invalid images use the supplied fallback glyph. Retail icons are not included in the app.
- The four stored defaults are Push, Pull, Speed and Sense. A slot appears and works only when its power is currently learned. Unavailable saved assignments remain stored but appear Empty in the editor and hide during gameplay, without revealing their names or icons. They return when available again. Explicitly choosing Empty keeps a slot empty.
- Open Lock → Force, or tap a quick slot while editing, to assign it. The picker offers Empty and currently learned powers only: Push, Pull, Speed, Sense, Absorb, Heal, Mind Trick, Protect, Drain, Grip, Lightning and Dark Rage. It does not reveal later powers. Picking a power already assigned elsewhere moves it to this slot and clears the former slot; it does not swap assignments.
- Empty slots do nothing and stay invisible during gameplay. The editor shows four outlined placeholders so you can position slots before learning powers. Until then, their pickers offer only Empty. Learned powers can be assigned later. Assignments are saved separately for SP and MP.
- Quick slots use their assigned power directly without changing your selected power. Grip, Lightning and Drain use stock press/hold/release commands. Other quick powers activate once per press. Stock energy, cooldown and game rules still apply.
- Central Use and all quick slots share one touch owner. Competing touches are ignored; they do not activate later. Force arrows are also ignored while that owner is busy. Lift and press again to try another action. Editing, loss of availability, map/loadout changes and backgrounding cancel touch ownership and held powers; hardware co-holds keep their own input.
- MP has one pending one-shot command. A quick one-shot keeps touch Force busy until its finger lifts and that command is sent. Rapid taps during this interval are ignored. Hardware keeps stock behavior and can replace a pending generic command; touch adds no delayed activation queue.

### Edit the layout and aim

- Tap Lock below Console to edit. Held touch actions release, touch sticks and drag stop, and the game view dims. All controls appear for editing; physical gamepad and keyboard input keep their state.
- Drag a button, including Console or Lock, to move it. Its size stays fixed. Buttons may reach the physical screen edges, beyond the safe area or HUD gutters; the whole hitbox stays inside the drawable screen. An edge position can overlap a cutout or home indicator.
- Tap a quick slot without dragging to open its picker. Hidden or overlapping slots are also reachable through the Force tab.
- The Appearance / Aim / Force / Done group starts collapsed at screen center. Tap a tab to expand it; tap the same tab to collapse it. The expanded group centers and clamps inside safe bounds. Short screens scroll the panel while Done stays visible. Panel touches and extra fingers cannot drag or activate controls underneath.
- Appearance offers opacity choices from 20% to 100%. Aim offers Joystick / Free drag, that mode's sensitivity, and a joystick-only curve. Custom saved opacity and sensitivity stay unchanged until you pick a value. Buttons stay fully visible while editing.
- Sensitivity choices are .25, .5, .75, 1, 1.25, 1.5, 2, 3 and 4. Joystick and Free drag save separate values. Mode, curve or sensitivity changes cancel existing touch gestures before the next game command; press again to resume. Physical controller response is unchanged.
- Tap Done, or tap Lock without dragging, to save and return to play. Opening a menu or console, changing maps, restarting input/video, hiding controls or backgrounding also saves and closes the editor. SP and MP save separate layouts.
- Existing saved layouts migrate while preserving their prior on-screen centers where possible, then clamp after resize or rotation. `touchLayoutReset` restores positions, including Console; it keeps Force assignments, aim and opacity settings.

### Touch bindings

- Pad sends TAB; Chat sends Y. Both follow your normal key rebinds. An unbound TAB or Y does nothing. Restore Pad with `bind TAB datapad` in SP or `bind TAB +scores` in MP; restore MP Chat with `bind y messagemode`.
- Empty AUX16–27 bindings regain their defaults at startup after saved settings and autoexec. Nonempty rebinds are kept. Quick slots use their direct assigned-power actions rather than AUX binds.

| Button | Key | Default bind | Action |
| --- | --- | --- | --- |
| Attack | `AUX16` | `+attack` | Attack |
| Alt attack | `AUX17` | `+altattack` | Alternate attack |
| Jump | `AUX18` | `+moveup` | Jump |
| Crouch | `AUX19` | `+movedown` | Crouch |
| Use | `AUX20` | `+use` | Use |
| Force use | `AUX21` | `+useforce` | Use selected Force power |
| Force next | `AUX22` | `forcenext` | Next Force power |
| Style | `AUX23` | `saberAttackCycle` | Change saber style |
| Previous weapon | `AUX24` | `weapprev` | Previous weapon |
| Next weapon | `AUX25` | `weapnext` | Next weapon |
| Saber | `AUX26` | `weapon 1` | Select saber or toggle its blade |
| Force previous | `AUX27` | `forceprev` | Previous Force power |
| Pad | TAB | `datapad` (SP), `+scores` (MP) | Datapad / held scoreboard |
| Chat | Y | `messagemode` (MP) | Chat |

- Advanced Force Use rebinds keep stock command behavior. Cancellation before AUX21 binding expansion suppresses that press. Arbitrary custom text already expanded into the command buffer, including plain commands, mixed binds, `vstr` or `exec`, can still execute after touch cancellation. The default `+useforce` and stock held-button commands retain touch-source release handling.

### Gamepad, keyboard and mouse

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

- Keep `in_joystick 1` and `j_twinStick 1` for the normal move/pad stick path. Touch aim is independent of these controller switches. With `in_touchControls 2`, a deflected filtered pad stick takes priority over the matching touch pair; move and aim are separate.
- iPad keyboard and mouse use the normal key and mouse binds. Text input stays active for the console unless the console button hid its soft keyboard, MP chat, menus with a hardware keyboard, and menus where the console button requested the soft keyboard. It is off in gameplay and cinematics. Keyboard disconnect hands active text entry to the soft keyboard. Input and video restarts restore this state.
- Rebind in the console. For example, `bind AUX16 +altattack` changes Attack, `bind TAB +scores` changes Pad, and `bind y messagemode` changes Chat. Use a `JOY` key to change a pad bind.
- Desktop and sanitizer checks exercise geometry, gestures, reporting helpers, visibility, input/command timing, aim rates and embedded icon decoding using synthetic input. Real touch feel, reach, icon appearance, restart/server behavior, Xbox / DualSense, Smart Keyboard and mouse remain device checks.

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
- `in_touchAimMode 0`: default Joystick; `1` selects Free drag. Joystick turns at up to 210 degrees/second on each axis times its sensitivity. Free drag turns .20 degrees per view point times its sensitivity, without a frame-time multiplier.
- `in_touchAimCurve 2`: default Precise. After the radial deadzone, these are generic deterministic curve approximations, not Sony firmware equivalents:

| Value | Curve | Response after deadzone (`q` from 0 to 1) |
| --- | --- | --- |
| `0` | Default | `q` |
| `1` | Quick | `sqrt(q)` |
| `2` | Precise | `q^1.6` |
| `3` | Steady | `.6*q`, including a lower maximum speed |
| `4` | Digital | Full response whenever `q > 0` |
| `5` | Dynamic | `q*q*(3-2*q)` |

- The joystick uses an 8% radial deadzone, then remaps the remaining distance to `q`. Curves affect touch joystick aim only.
- `in_touchLookScale 1.0`: saved Joystick sensitivity. `in_touchDragScale 1.0`: separate saved Free drag sensitivity. Effective finite values clamp to `.1`–`10`; nonfinite values use `1`. Neither setting changes physical pad look. Old look-scale values survive, but custom `j_lookSpeed` or keyboard turn-speed values no longer change touch aim speed.
- `in_touchOpacity 1.0`: default touch opacity, clamped to `0`–`1`. `0` hides drawing but leaves touch input active. It does not change availability.
- `in_touchLayout`: saved versioned positions for this app; empty means defaults. Use Lock to edit or `touchLayoutReset` to restore positions.
- `in_touchForceSlots`: saved assignments, default `v1:1,2,3,4`. IDs: `0` Empty; `1` Push; `2` Pull; `3` Speed; `4` Sense; `5` Absorb; `6` Heal; `7` Mind Trick; `8` Protect; `9` Drain; `10` Grip; `11` Lightning; `12` Dark Rage. Prefer the learned-only editor picker to assign them.
- Touch mode, aim mode/curve, both sensitivities, opacity, layout and assignments are saved separately per app. Example console/autoexec lines: `seta in_touchControls 2`, `seta in_touchAimMode 1`, `seta in_touchDragScale 1.25`, `seta in_touchOpacity 0.5`.
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
- Console text, chat and default touch positions keep clear of the camera cutout and home indicator in both landscape orientations. Edited touch buttons may reach physical screen edges. `r_aspectSafeLeft`, `r_aspectSafeTop`, `r_aspectSafeRight` and `r_aspectSafeBottom` are read-only values in drawable pixels for reference.
- iPads below the 1.5 band get no new safe margins for 2D. The Face ID iPad's roughly 20 pt bottom inset is unchanged behavior, not a bug.
- The HUD stays centered. Touch buttons can use the side gutters or be moved elsewhere in the drawable screen.
- Menu cursor taps follow the menu mapping. Touch-button placement uses the full drawable screen independently of HUD gutters.
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
