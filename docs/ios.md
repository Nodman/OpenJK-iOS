# OpenJK on iOS / iPadOS

## What it is

- OpenJK SP runs Jedi Academy single player. OpenJK MP is the multiplayer client.
- Both apps use rd-vanilla through gl4es. Apple's OpenGL ES (EAGL) is the default provider. ANGLE over Metal is an opt-in; see [Choose a provider](#choose-a-provider).
- Game files are not included. Copy them from your own retail game.
- OpenJK is not affiliated with Disney, Lucasfilm or Raven Software. You need your own copy of the game.

## Devices

- iOS / iPadOS 17.0 or later. A12 or newer. Arm64 only.
- iPhone XR / XS and later, including SE 2 / 3.
- iPad 8th generation or later, Air 3 or later, mini 5 or later, and iPad Pro models from 2018 or later.
- A11 and older, including A10 / A10X iPads, are outside the supported device list.
- ProMotion screens: iPhone 13 Pro through 17 Pro Max, iPhone 17, iPhone Air, and iPad Pro models from 2018 or later. Fresh configs default to 120 fps on these panels and 60 elsewhere; saved choices are kept.
- On iPhone, EAGL stays near 60 fps. 120 needs the ANGLE opt-in. iPad Pro reaches 120 on EAGL.
- 3 GB devices were not tested.

## Install

The IPAs are unsigned. A sideloading tool signs them with your own Apple ID. Pick one:

1. **SideStore** (recommended): set it up once with a computer using [iloader](https://iloader.app/), install the LocalDevVPN app, then refresh on the phone over Wi-Fi with no computer. Add the [OpenJK source](https://raw.githubusercontent.com/Nodman/OpenJK-iOS/ios/altstore/source.json) and install OpenJK SP, OpenJK MP, or both. [SideStore guide](https://docs.sidestore.io/).
2. **AltStore Classic**: run AltServer on a Mac or PC, install AltStore (iOS 17.4 or later), add the same source, install. AltServer refreshes the apps while the phone is on the same Wi-Fi or USB. [AltStore guide](https://faq.altstore.io/).
3. **Sideloadly, Impactor or Xcode**: download the IPA from [GitHub Releases](https://github.com/Nodman/OpenJK-iOS/releases), open it in the tool, sign in with your Apple ID. Re-sign with the computer every 7 days. Use this when SideStore or AltStore cannot sign in.

Limits set by Apple, not by the tools:

- A free Apple ID: apps expire after 7 days and only 3 sideloaded apps can be active. SideStore or AltStore, OpenJK SP and OpenJK MP use all three slots.
- A paid Apple Developer account ($99/year): apps last one year, no 3-app limit.
- If signing fails with "provisioning profile is banned", use a fresh Apple ID or a paid account.
- Each release includes `SHA256SUMS` for the IPAs.

## Copy game files

- Launch each app once to create its `base` folder.
- From your Jedi Academy installation's `GameData/base`, copy `assets0.pk3`, `assets1.pk3`, `assets2.pk3`, and `assets3.pk3`.
- In Files, open **On My iPhone / iPad → OpenJK SP → base** or **OpenJK MP → base**. Put the files directly in `base`.
- From a computer: connect the device by USB, open it in Finder (macOS) or iTunes (Windows), go to the Files tab, and drag the pk3 files into the app's `base` folder.
- SP and MP each need their own copy. Their app folders are separate.
- Return to the app and tap **Check again** in the **Game files needed** box.
- Startup checks for `assets0.pk3` and `default.cfg` for SP or `mpdefault.cfg` for MP. The cfg can be inside a pk3. The retail files supply it; `assets0.pk3` alone is not enough.
- Game pk3 files are excluded from backup. Keep your original copies.
- Put your autoexec in the same app's `base` folder: `autoexec.cfg` for MP, `autoexec_sp.cfg` for SP.

## Build yourself

- Use macOS, Xcode 26.x, and CMake 3.28 or later.
- From a checkout of the `ios` branch, run `scripts/build-ios.sh`. A clean configure downloads the pinned ANGLE package; see [ANGLE dependency](ios-angle.md).
- Without a team ID, it makes unsigned IPAs for a sideloader to sign. For a signed build, run `OPENJK_IOS_TEAM_ID=<your team> scripts/build-ios.sh`.
- IPAs: `build-ios/dist/OpenJK-SP-<version>.ipa` and `OpenJK-MP-<version>.ipa`, with `SHA256SUMS`. `OPENJK_IOS_VERSION` sets the version (default `0.1.0`).
- CI uploads the `ipas` artifact. Tags named `ios-v*` publish both IPAs and checksums to a GitHub Release.
- More in [developer notes](ios-dev.md).

## Controls

- Put a finger on the lower left to move, or on free space on the right to aim. The move stick appears where you touch.
- Aim defaults to Free drag: move a finger to turn, stop moving to stop. The Aim tab also offers Joystick: a small deflection turns slowly, a larger one faster; return to center or lift to stop.
- Attack, alternate attack and jump form an arc on the right. Attack, alternate attack, Force Use, Jump and Crouch press immediately; hold to keep the action. Drag any of them past a small threshold to aim while holding.
- The first aiming finger keeps aim until it lifts. A deflected physical right stick takes priority over touch aim.
- Attack shows a swing glyph for sabers and a trigger glyph for guns. Style shows the saber stance color: blue fast, yellow medium, red strong.
- Use sits at the bottom left. Crouch and Style sit near the bottom right. Previous weapon, Saber and Next weapon run along the top right; Saber selects the saber or toggles its blade. Force Previous / Use / Next sit below them, with four quick slots to their left.
- Menu, MP Chat and Pad sit at the top left. Menu sends Escape. Chat opens global chat and the keyboard (MP only). Pad opens the SP datapad, or holds the MP scoreboard until you lift.
- Buttons hide when not useful: Saber when unowned, weapon arrows with one weapon, Force arrows when cycling has no effect, Style when no stance change is legal. Positions stay fixed. Death, spectator and intermission keep basic actions and sticks.
- Tap or drag elsewhere in menus to use the cursor. Tap Console to open the console and keyboard; tap again to close. Hold Console for 500 ms to toggle the soft keyboard for menu fields.
- Full-screen cinematics and SP in-game videos hide all controls; tap anywhere to skip.

### Force selection and quick slots

- Previous / Next follow the stock Force order. Use activates the selected power. Icons load from your game files; missing ones use a fallback glyph.
- The four default slots are Push, Pull, Speed and Sense. A slot shows only while its power is learned. Empty slots stay hidden in play.
- To assign a slot, open Lock → Force, or tap the slot while editing. The picker offers Empty and learned powers only. Picking a power already in another slot moves it.
- Quick slots use their power directly without changing your selected power. Grip, Lightning and Drain hold while pressed; other powers fire once per press. Stock energy, cooldown and game rules apply.
- Assignments are saved separately for SP and MP.

### Edit the layout and aim

- Tap Lock to edit. Held actions release and the game view dims.
- Drag a button, including Console or Lock, to move it. Sizes are fixed. Buttons may reach the screen edges.
- Tap a quick slot without dragging to open its picker.
- The Appearance / Aim / Force / Done group starts collapsed at screen center. Tap a tab to expand it.
- Appearance: opacity slider 0–100% and Reset layout (positions only). Aim: Joystick / Free drag, horizontal and vertical sensitivity from .25 to 4, and a joystick response curve. Drag across a slider to change it live.
- Tap Done, or Lock without dragging, to save. Opening a menu, changing maps or backgrounding also saves. SP and MP keep separate layouts.
- Phone and tablet have different default layouts. `touchLayoutReset` in the console restores positions.

### Touch bindings

- Pad sends TAB; Chat sends Y. Both follow your rebinds. Restore Pad with `bind TAB datapad` in SP or `bind TAB +scores` in MP; restore Chat with `bind y messagemode`.
- Empty AUX16–27 bindings regain their defaults at startup. Rebinds are kept.

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

- Keep `in_joystick 1` and `j_twinStick 1` for the normal stick path. Touch aim is independent of these switches.
- iPad keyboard and mouse use the normal key and mouse binds. The console button can show or hide the soft keyboard when a hardware keyboard is attached.
- Rebind in the console. For example, `bind AUX16 +altattack` changes Attack; use a `JOY` key to change a pad bind.

## Settings

Enter commands in the console, or put `seta` lines in that app's autoexec (`base/autoexec.cfg` for MP, `base/autoexec_sp.cfg` for SP). SP and MP save settings separately.

### Choose a provider

- Video → `Graphics backend` offers EAGL / ANGLE. The choice is saved at once; no Apply is needed.
- Fully close the app from the app switcher, then open it again. A provider change applies only at the next launch; Apply and `vid_restart` do not switch it.
- The help line under the row shows the provider in use and whether a relaunch is pending.
- Console alternative: `seta r_glesProvider angle` (or `eagl`), then relaunch the same way. SP and MP keep separate choices.
- To check, enter `r_glesProvider` (your choice) and `r_glesProviderActive` (read-only, the provider in use). The startup log also prints `iOS display: provider=<active> requested=<choice>`.
- If the console is unreachable, put the `seta` line in that app's autoexec in Files while it is closed.
- If ANGLE fails during setup, the app tries EAGL in the same launch.

### Request 60 or 120 fps

- Video → `120 Hz (ProMotion)` offers Off (60) / On (120) on 120 Hz panels. Apply restarts video. It does not change the provider; use `Graphics backend` for that.
- Console: `seta com_maxfps 60|120` (choose one value).
- Fresh configs default to 120 on 120 Hz panels, 60 elsewhere. Saved choices are kept. `reset com_maxfps` returns to the panel default.
- A rate change applies at once. At Auto resolution (`r_renderScale 0`), run `vid_restart` so the scale matches.
- 120 needs a capable panel. Low Power Mode, workload and heat can lower it. The game drops to 60 at Serious thermal state and 30 at Critical.

### Video

- Video → Resolution offers Auto, Native, 3/4 and Native/2 with the predicted pixel size. Native is the default. Apply restarts the video.
- Console: `seta r_renderScale 0|1|0.75|0.5` (choose one value), then `vid_restart`. `0` is Auto; `1` is Native; `0.5`–`1.0` sets a fixed scale. Lower values reduce detail and GPU load.
- Auto picks a scale from the GPU family, lower at 120 fps. See [developer notes](ios-dev.md) for the rule.
- Existing saved configs keep their resolution and FPS values. To adopt the defaults on a 120 Hz panel, enter `seta r_renderScale 1` and `seta com_maxfps 120`, then `vid_restart`.
- Video → `FPS counter` offers Off / On. On shows a small gold frame-rate readout at the top right in a map.
- Dynamic glow is unavailable on the GL path, so its More Video row is hidden. Light flares can be enabled in More Video; they cost frame time on tile GPUs.

### Touch

- `in_touchControls 0`: hide touch controls except the console button. `1`: default; hide them when a pad is connected, or a hardware keyboard and mouse/trackpad are both connected. `2`: keep them visible with these devices.
- `in_touchAimMode 1`: default Free drag; `0` selects Joystick.
- `in_touchAimCurve 2`: default Precise. Joystick aim only; `q` is the stick distance after an 8% deadzone:

| Value | Curve | Response |
| --- | --- | --- |
| `0` | Default | `q` |
| `1` | Quick | `sqrt(q)` |
| `2` | Precise | `q^1.6` |
| `3` | Steady | `.6*q`, lower top speed |
| `4` | Digital | Full response whenever `q > 0` |
| `5` | Dynamic | `q*q*(3-2*q)` |

- `in_touchLookScale 1.0` / `in_touchLookScaleY 0.6`: Joystick horizontal / vertical sensitivity. `in_touchDragScale 3.0` / `in_touchDragScaleY 1.8`: Free drag horizontal / vertical sensitivity. Range `.1`–`10`.
- `in_touchOpacity 1.0`: touch opacity, `0`–`1`. `0` hides drawing but keeps input active.
- `in_touchLayout`: saved positions; empty means defaults. `in_touchForceSlots`: saved quick-slot assignments, default `v1:1,2,3,4`. Prefer the editor to change both.
- Example autoexec lines: `seta in_touchControls 2`, `seta in_touchAimMode 1`, `seta in_touchDragScale 1.25`, `seta in_touchOpacity 0.5`.

### Gamepad

- `j_lookSpeed 1.5`: pad look scale. Higher turns faster.
- `j_deadzone 0.15`: stick dead zone. Raise it if a stick drifts.
- `j_lookExponent 2.0`: right-stick response curve.

### Quality defaults

- `r_texturebits 32` on all devices. `r_picmip 1` on devices with about 3.5 GB RAM or less. `r_surfaceSprites 0` on A12 / A13 GPUs.
- `r_surfaceSprites` and `r_texturebits` are not saved when equal to the engine default. For example, `seta r_surfaceSprites 1` on A12 is lost at restart. Put such overrides in the app's autoexec.

### Aspect-correct 2D, FOV and safe area

- On screens wider than 1.5:1, the 640×480 menus, HUD and console keep their 4:3 shape, centered with side gutters. The 3D view fills the screen. iPads below 1.5:1 keep the old stretched 2D.
- `r_aspectCorrect 1`: iOS default. `0` restores the old stretched 2D for mods or menus that already compensate.
- `r_aspectCorrectMinRatio 1.5`: minimum screen ratio for aspect-correct 2D.
- `r_aspectStretchBackdrops 0`: loading art and menu backgrounds fit 4:3 with black gutters. `1` stretches them.
- `cg_fovAspectAdjust 1`: keeps the 4:3 vertical field of view on wide screens. `0` restores the old crop. `cg_fov` stays `80`.
- Console text, chat and default touch positions keep clear of the camera cutout and home indicator. Edited touch buttons may reach the physical edges.

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
- **No sound:** check the silent switch.
- **Disconnected after background:** time away longer than `cl_timeout` disconnects a remote MP session. Rejoin the server.
