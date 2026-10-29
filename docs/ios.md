# OpenJK on iPhone and iPad

## What it is

- **OpenJK SP** plays the Jedi Academy story. **OpenJK MP** plays online multiplayer.
- The apps contain no game files. You need your own copy of the game (Steam, GOG or disc).
- OpenJK is not affiliated with Disney, Lucasfilm or Raven Software.

## Will it run on my device

- Yes on iPhone XR, XS or newer, including SE 2 and 3, on iOS 17 or later.
- Yes on iPad 8th generation, iPad Air 3, iPad mini 5, any iPad Pro from 2018, or newer, on iPadOS 17 or later.
- Older models (A11 chips and older) are not supported. A10 iPads can install the app but were not tested.

## Install

Apple does not sell this app. You "sideload" it with a free tool that signs it with your Apple ID.

Recommended: SideStore. Once set up, it refreshes the apps on its own over Wi-Fi.

1. On a computer, install SideStore on your phone with [iloader](https://iloader.app/). This is a one-time step. The [SideStore guide](https://docs.sidestore.io/) walks through it.
2. On the phone, turn on Developer Mode: **Settings → Privacy & Security → Developer Mode**. Restart when asked.
3. Open SideStore, go to **Sources**, tap **+**, and paste this address:

   `https://raw.githubusercontent.com/Nodman/OpenJK-iOS/ios/altstore/source.json`

4. Install **OpenJK SP**, **OpenJK MP**, or both.
5. Every 7 days, open SideStore and tap **Refresh All**. Otherwise the apps stop opening.

Other ways:

- **AltStore Classic**: install AltServer on a Mac or PC and AltStore on the phone, then add the same source. AltServer refreshes the apps while the phone is on the same Wi-Fi or plugged in. [AltStore guide](https://faq.altstore.io/).
- **Sideloadly, Impactor or Xcode**: download the IPA file from [Releases](https://github.com/Nodman/OpenJK-iOS/releases), open it in the tool and sign in with your Apple ID. You must plug the phone into the computer again every 7 days.

Limits set by Apple, not by the tools:

- With a free Apple ID, apps expire after 7 days and only 3 sideloaded apps can be installed at once. SideStore, OpenJK SP and OpenJK MP use all three.
- With a paid Apple Developer account (99USD a year), apps last a year and there is no 3-app limit.

## Copy the game files

1. Open OpenJK SP once. It shows a **Game files needed** box. Leave the app open.
2. On your PC, open the game folder. On Steam: right-click the game → **Manage** → **Browse local files**, then open `GameData`, then `base`.
3. Send these four files to the phone: `assets0.pk3`, `assets1.pk3`, `assets2.pk3`, `assets3.pk3`. Use iCloud Drive, AirDrop, or a USB cable with Finder on a Mac or the Apple Devices app on Windows (**Files** tab).
4. On the phone, open the **Files** app and go to **On My iPhone** (or **On My iPad**) → **OpenJK SP** → **base**. Move the four files into `base`. Not into a subfolder.
5. Back in the app, tap **Check again**.

Repeat for OpenJK MP. Each app has its own `base` folder and needs its own copy of the files.

Deleting an app deletes its game files and saved games. Updates install over the app and keep them.

## Play

Left thumb moves. Right thumb aims. Buttons sit around the edges.

- **Move**: put a finger on the lower left. The stick appears where you touch.
- **Aim**: drag a finger on free space on the right.
- **Attack arc** on the right: Alternate attack, Attack and Jump. **Both attacks** holds both attack buttons at once, for moves that need two keys on a keyboard.
- **Crouch** and **Style** (saber stance) sit near the bottom right. **Use** sits at the bottom left.
- **Weapons**: Previous, Saber and Next along the top right. **Force**: Previous, Use and Next below them, with four quick slots on the upper left.
- **Menu**, **Chat** (multiplayer) and **Pad** (datapad, or the scoreboard in multiplayer) sit at the top left.
- **Camera** switches between first and third person. It sits under **Lock** and shows when Lock shows.
- Buttons you cannot use right now are hidden. The hidden spots stay empty.
- Attack buttons fire at once. Keep holding to keep attacking. Drag an attack button to aim while holding.
- In cutscenes and videos, tap anywhere to skip.

**Edit the layout**: tap the **Lock** button. Drag any button to move it. The settings panel sits at the top. Drag its tab bar to move it. Tap a tab for more:

- **Appearance**: button opacity and Reset layout.
- **Aim**: Free drag (default) or Joystick, and sensitivity.
- **Force**: choose which Force powers the four quick slots hold.
- **Haptics**: turn the tap feel on or off.

Tap **Done** (bottom of the panel) or **Lock** again to save. SP and MP keep separate layouts.

**Gamepad**: left stick moves, right stick looks, A jumps, B crouches, X uses, Y uses the selected Force power, triggers attack, bumpers change weapons, D-pad changes Force powers and saber style. In menus, A selects and B goes back. Touch buttons hide while a pad is connected.

**Keyboard and mouse** (iPad) work like on a PC.

## Settings in the game menu

Open **Setup → Video** from the main menu:

- **Resolution**: Auto, Native, 3/4 or Native/2. Lower is faster. Tap Apply.
- **120 Hz (ProMotion)**: on devices with a 120 Hz screen. New installs start at 120 on those screens.
- **Graphics backend**: EAGL (default) or ANGLE. ANGLE reaches 120 fps on iPhone; EAGL stays near 60 on iPhone and reaches 120 on iPad Pro. A change takes effect after you fully close the app from the app switcher and open it again.
- **FPS counter**: shows the frame rate at the top right.

Settings and saved games are kept separately for SP and MP.

## Troubleshooting

- **The app no longer opens after a week.** The signature expired. Open SideStore or AltStore and tap **Refresh All**, or sign the IPA again from the computer.
- **"Maximum number of apps" when installing.** A free Apple ID allows 3 sideloaded apps, and SideStore counts as one. Remove one, or install only SP.
- **"Untrusted Developer" when opening.** Settings → General → VPN & Device Management → tap your Apple ID → Trust.
- **The app asks for Developer Mode.** Settings → Privacy & Security → Developer Mode → On. Restart.
- **"Provisioning profile is banned."** Sign with a different Apple ID, or use Sideloadly.
- **"Game files needed" keeps showing.** The files are in a subfolder or in the other app's folder. Put all four `assets` files directly in that app's `base` folder, then tap **Check again**.
- **Black screen after choosing ANGLE.** Close the app from the app switcher. In the Files app, open the app's `base` folder and create a text file named `autoexec_sp.cfg` (SP) or `autoexec.cfg` (MP) with the line `seta r_glesProvider eagl`. Open the app again.
- **Low frame rate or stutter.** Video → Resolution → 3/4 or Native/2. Turn 120 Hz off. A hot device slows down on its own and recovers when it cools.
- **Touch buttons are gone.** A controller, or a keyboard and mouse, is connected. Disconnect it. To keep the buttons anyway, see the advanced guide.
- **Cannot type in a menu field.** Hold the **Console** button for half a second to show the keyboard.
- **No sound.** Check the silent switch.
- **Multiplayer: disconnected after switching apps.** The server dropped you while the app was in the background. Rejoin.
- **Multiplayer: cannot join a server.** Extra files in the `base` folder can block "pure" servers. Keep only the four `assets` files there. Servers that need their own client mods do not work.

## More

- [Advanced settings](ios-advanced.md): console commands, every button and key binding, and all options.
- [Developer notes](ios-dev.md): building the apps yourself, tests and internals.
