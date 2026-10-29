# OpenJK on iOS: advanced settings

Everything here uses the in-game console. Players who stick to the menus can skip this page.

## Console and autoexec

- Tap the **Console** button (top left on iPhone, top right on iPad) to open the console and keyboard. Tap it again to close.
- Type a command and press return. `seta name value` changes a setting and saves it.
- To apply settings at every launch, put the `seta` lines in a text file in the app's `base` folder: `autoexec_sp.cfg` for SP, `autoexec.cfg` for MP. You can create it in the Files app.
- SP and MP keep separate settings.

## Touch buttons and bindings

- Each touch button presses a key. Rebind the key to change what the button does, for example `bind AUX16 +altattack`.
- Pad presses TAB and Chat presses Y, so they follow your rebinds. Restore Pad with `bind TAB datapad` (SP) or `bind TAB +scores` (MP); restore Chat with `bind y messagemode`.
- Empty AUX16–29 bindings regain their defaults at startup. Rebinds are kept.
- `touchLayoutReset` restores the default button positions. Phones and tablets have different defaults.

| Button | Key | Default bind | Action |
| --- | --- | --- | --- |
| Attack | `AUX16` | `+attack` | Attack |
| Alt attack | `AUX17` | `+altattack` | Alternate attack |
| Both attacks | `AUX28` | `+attack;+altattack` | Attack and alternate attack together |
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
| Camera | `AUX29` | `toggle cg_thirdPerson` | Switch first / third person |
| Pad | TAB | `datapad` (SP), `+scores` (MP) | Datapad / held scoreboard |
| Chat | Y | `messagemode` (MP) | Chat |

Button behaviour:

- Attack, alternate attack, Both attacks, Force use, Jump and Crouch press immediately. Hold to keep the action. Drag any of them past a small threshold to aim while holding.
- The first aiming finger keeps aim until it lifts. A deflected physical right stick takes priority over touch aim.
- Attack shows a swing glyph for sabers and a trigger glyph for guns. Style shows the stance colour: blue fast, yellow medium, red strong.
- Buttons hide when not useful: Saber when unowned, weapon arrows with one weapon, Force arrows when cycling has no effect, Style when no stance change is legal. Death, spectator and intermission keep the basic actions and sticks.
- Both attacks sits lower-left of Alt attack on iPhone and below it on iPad.
- Console sits at the top left on iPhone and the top right on iPad. Lock sits on the left edge on iPhone and the top right on iPad. Camera sits under Lock.
- Camera shows with the basic actions. Menus, the console and cutscenes hide it.
- Hold Console for half a second to show or hide the soft keyboard for menu fields. With a hardware keyboard attached, the same hold shows or hides it.
- Full-screen videos and SP cutscenes hide all controls. SP cutscenes draw no letterbox bars on screens 1.5:1 or wider; iPads keep them.

### Force selection and quick slots

- Previous / Next follow the stock Force order. Use activates the selected power. Icons come from your game files; missing ones use a fallback glyph.
- The default slots are Push, Pull, Speed and Sense. A slot shows only while its power is learned.
- Assign a slot from Lock → Force, or tap the slot while editing. The picker offers Empty and learned powers only. Picking a power that sits in another slot moves it.
- Quick slots use their power without changing the selected one. Grip, Lightning and Drain hold while pressed; other powers fire once per press. Normal energy and cooldown rules apply.
- Assignments are saved separately for SP and MP: `in_touchForceSlots`, default `v1:1,2,3,4`. Prefer the editor.

### Layout editor

- Tap Lock to edit. Held actions release and the game view dims.
- Drag a button, including Console or Lock, to move it. Sizes are fixed. Buttons may reach the screen edges.
- Tap a quick slot without dragging to open its picker.
- Appearance: opacity 0–100% and Reset layout. Aim: Joystick or Free drag, horizontal and vertical sensitivity from .25 to 4, and a joystick response curve. Drag across a slider to change it live.
- Tap Done, or Lock without dragging, to save. Opening a menu, changing maps or backgrounding also saves.
- `in_touchLayout` holds the saved positions; empty means defaults.

### Touch settings

- `in_touchControls 0`: hide touch controls except the console button. `1` (default): hide them when a pad is connected, or when a hardware keyboard and a mouse or trackpad are both connected. `2`: keep them visible with these devices.
- `in_touchAimMode 1`: Free drag (default); `0`: Joystick.
- `in_touchAimCurve 2`: Joystick response, default Precise. `q` is the stick distance after an 8% dead zone:

| Value | Curve | Response |
| --- | --- | --- |
| `0` | Default | `q` |
| `1` | Quick | `sqrt(q)` |
| `2` | Precise | `q^1.6` |
| `3` | Steady | `.6*q`, lower top speed |
| `4` | Digital | Full response whenever `q > 0` |
| `5` | Dynamic | `q*q*(3-2*q)` |

- `in_touchLookScale 1.0` / `in_touchLookScaleY 0.6`: Joystick horizontal / vertical sensitivity. `in_touchDragScale 3.0` / `in_touchDragScaleY 1.8`: Free drag horizontal / vertical sensitivity. Range `.1`–`10`.
- `in_touchOpacity 1.0`: button opacity, `0`–`1`. `0` hides the drawing but keeps input active.
- Example autoexec lines: `seta in_touchControls 2`, `seta in_touchAimMode 1`, `seta in_touchDragScale 1.25`, `seta in_touchOpacity 0.5`.

## Gamepad, keyboard and mouse

- Left stick moves; right stick looks. In menus the left stick moves the cursor, A / Cross selects and B / Circle goes back.
- While a Controls item waits for a key, A and B send `JOY0` and `JOY1` so they can be bound. Start cancels the capture and cannot be bound.
- Pad button `n` is key `JOYn`. Start sends Escape. LT is `JOY21`, RT is `JOY22`. Rebind with `bind JOY2 +altattack` and so on.
- `j_lookSpeed 1.5`: look speed. `j_deadzone 0.15`: stick dead zone; raise it if a stick drifts. `j_lookExponent 2.0`: right-stick response curve.
- Keep `in_joystick 1` and `j_twinStick 1`. Touch aim does not depend on them.
- iPad keyboard and mouse use the normal key and mouse binds.

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

## Graphics

### Graphics backend

- The Video menu row saves the choice at once. It applies at the next launch: fully close the app from the app switcher and open it again. Apply and `vid_restart` do not switch it.
- Console: `seta r_glesProvider angle` or `seta r_glesProvider eagl`, then relaunch the same way.
- `r_glesProviderActive` (read-only) shows the backend in use. The startup log prints `iOS display: provider=<active> requested=<choice>`.
- If ANGLE fails during setup, the app falls back to EAGL in the same launch.
- Both apps embed ANGLE even when EAGL runs. See the [ANGLE note](ios-angle.md).

### Frame rate

- `seta com_maxfps 60` or `120`. New installs default to 120 on 120 Hz screens and 60 elsewhere. `reset com_maxfps` returns to the screen default.
- A rate change applies at once. At Auto resolution, run `vid_restart` so the scale matches.
- Low Power Mode, load and heat can lower the rate. The game drops to 60 at the Serious thermal state and 30 at Critical, and warns in the console.
- On iPhone, EAGL stays near 60 fps; 120 needs ANGLE. iPad Pro reaches 120 on EAGL.

### Resolution and quality

- `seta r_renderScale 0|1|0.75|0.5`, then `vid_restart`. `0` is Auto, `1` is Native, `0.5`–`1.0` is a fixed scale.
- Auto picks a scale from the GPU family, lower at 120 fps. See the [developer notes](ios-dev.md).
- Existing configs keep their resolution and frame-rate values. To adopt the defaults on a 120 Hz screen: `seta r_renderScale 1`, `seta com_maxfps 120`, `vid_restart`.
- Defaults: `r_texturebits 32` everywhere; `r_picmip 1` on devices with about 3.5 GB of RAM or less; `r_surfaceSprites 0` on A12 and A13.
- `r_surfaceSprites` and `r_texturebits` are not saved when they equal the device default. Put such overrides in the autoexec.
- Dynamic glow is unavailable on the GL path, so its More Video row is hidden. Light flares cost frame time on mobile GPUs.
- Shadows: keep `cg_shadows 1`. `cg_shadows 2` reads back the full screen every frame. Keep `r_measureOverdraw 0`.
- Refraction: keep `cg_renderToTextureFX 0` and `cg_saberTrail 1`. The refraction variants copy the screen mid-frame and cost frames.
- Crash while loading on a 3 GB device: put `seta r_texturebits 16` in the autoexec. 3 GB devices were not tested.

### Screen shape and field of view

- On screens 1.5:1 or wider, the 640×480 menus, HUD and console keep their 4:3 shape, centred with side gutters. The 3D view fills the screen. iPads below 1.5:1 keep the stretched 2D.
- `r_aspectCorrect 1`: iOS default. `0` restores the stretched 2D for mods or menus that already compensate. `r_aspectCorrectMinRatio 1.5`: minimum screen ratio for the corrected 2D.
- `r_aspectStretchBackdrops 0`: loading art and menu backgrounds fit 4:3 with black gutters. `1` stretches them.
- Scope, binocular and goggle masks keep their 4:3 shape and black out the side gutters. Force Sight, camera and droid overlays cover the whole screen.
- `cg_fovAspectAdjust 1`: keeps the 4:3 vertical field of view on wide screens. `0` restores the old crop. `cg_fov` stays `80`.
- Console text, chat and default touch positions keep clear of the camera cutout and home indicator. Edited buttons may reach the physical edges.

### Camera and weapon view

- iPhone only: `cg_thirdPersonRange 130`, `cg_fovViewmodel 100`, `cg_gunX 5`. iPad keeps the engine values `80`, `0`, `0`.
- `reset <name>` returns to the device default. A saved value from older console testing wins until then, for example `reset cg_thirdPersonRange`.

## Other behaviour

- Backgrounding pauses frames and sound, releases held input and saves the config. MP disconnects from a remote server if the time away exceeds `cl_timeout`.
- Audio follows the silent switch and stops other audio.
- The screen does not sleep while the app runs.
- Game pk3 files are excluded from iCloud backup. Keep your original copies.
- Startup checks for `assets0.pk3` and `default.cfg` (SP) or `mpdefault.cfg` (MP). The retail pk3 files supply the cfg.
- Multiplayer: allow the Local Network prompt on first LAN use. LAN browsing is not supported; connect by server address.
