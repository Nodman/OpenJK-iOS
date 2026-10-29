OpenJK for iOS and iPadOS: Jedi Academy single player (OpenJK SP) and multiplayer (OpenJK MP). You need your own copy of the game files.

## What's new in 0.2.0

- Camera button under Lock: switch between first and third person with one tap.
- Haptic click on touch buttons (iPhone). Turn it off in Lock → Haptics, or with `in_touchHaptics 0`.
- Settings panel: drag its tab bar to move it anywhere; the spot is saved. Done is a button at the bottom of the panel. New Haptics tab.
- Weapon and Force arrows accept taps a little outside their drawn edge.
- iPhone defaults: third-person camera sits further back (`cg_thirdPersonRange 130`), the weapon model is smaller and further away (`cg_fovViewmodel 100`, `cg_gunX 5`). iPad keeps the engine values.
- Upgrading from 0.1.0: values still at the old default move to the new iPhone defaults once, on first launch. Values you changed are kept. Your button layout is kept; Camera is placed clear of moved buttons.

## Install

1. Install SideStore or AltStore, then add this source: `https://raw.githubusercontent.com/Nodman/OpenJK-iOS/ios/altstore/source.json`
2. Or download the IPA below and sign it with Sideloadly, Impactor or Xcode.
3. Launch the app once, then copy `assets0.pk3` to `assets3.pk3` from your game's `GameData/base` into the app's `base` folder in Files.

Requirements: iOS or iPadOS 17.0 or later, A12 chip or newer. `SHA256SUMS` lists the checksums of the IPAs.

Full guide: https://github.com/Nodman/OpenJK-iOS/blob/ios/docs/ios.md
Developer notes: https://github.com/Nodman/OpenJK-iOS/blob/ios/docs/ios-dev.md
