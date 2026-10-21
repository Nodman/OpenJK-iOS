# ANGLE dependency and local opt-in status

This note accompanies the [iOS guide](ios.md). ANGLE over Metal is an explicit local opt-in. EAGL and fresh/reset 60 fps remain the defaults; saved choices survive. Practical SP/MP playtests passed, but P10's full qualification remains open.

## Dependency delivery

The application always embeds ANGLE, including when EAGL is selected at runtime. A clean iOS configure needs the four exact assets pinned in [angle-consumer.json](../scripts/ios/angle-consumer.json): `manifest.json`, `SHA256SUMS`, `libEGL.xcframework.zip` and `libGLESv2.xcframework.zip`. An older RGB-batch package is not interchangeable with the current RGB direct-staging package.

The current identity is `angle-3c3c1746733c-rgb-playable-direct-r1-415d50aa73e26917-b386b9e0a9eb3237-ios-arm64`. The exact tested package is available from the [dependency release](https://github.com/Nodman/OpenJK-iOS/releases/tag/angle-3c3c1746733c-rgb-playable-direct-r1-415d50aa73e26917-b386b9e0a9eb3237-ios-arm64), with provenance commit [`f5e36d8187999560049cdd42b344ad3f2c2d5db1`](https://github.com/Nodman/OpenJK-iOS/commit/f5e36d8187999560049cdd42b344ad3f2c2d5db1). A clean configure downloads the four pinned assets over HTTPS and validates their hashes, manifest identity and framework binaries. No ANGLE source build or local package is required:

```sh
unset OPENJK_IOS_ANGLE_LOCAL_ASSETS
scripts/build-ios.sh
```

The release is a prerelease and is not Latest. GitHub reports `immutable: false`; the consumer's trusted hashes enforce the expected bytes, without server-enforced release immutability. The original 11 recipe inputs and assembly evidence are preserved at the provenance commit. The app integration's recipe helper differs as described below.

An optional offline/local override must contain the same exact package and pass every existing asset, manifest and framework hash check:

```sh
export OPENJK_IOS_ANGLE_LOCAL_ASSETS=/absolute/path/to/exact-package
python3 scripts/ios/angle-consumer.py prepare \
  --destination "$PWD/build-angle-consumer-check" \
  --local-assets "$OPENJK_IOS_ANGLE_LOCAL_ASSETS"
scripts/build-ios.sh
```

Use a new destination for the check and a clean app build directory. The build script produces unsigned IPAs unless a team is explicitly supplied. Retail game assets are not required for compilation.

The hosted dependency is available to clean checkouts and CI through the default consumer path. A fresh local unsigned iOS configure with PERF OFF and no local override verified real HTTPS delivery and the exact framework hashes; the earlier full unsigned Release SP/MP build and module/app checks passed with the identical package. This local verification does not claim that a hosted GitHub Actions iOS job has executed. Dependency release-event jobs were skipped by the release guards; no app IPA release is implied.

The separate dependency workflow builds new packages; it is not needed to consume this release and does not guarantee a runner with the exact source-build host. A future source rebuild needs provenance and binary inspection, explicit acceptance of its new identity, and a deliberate consumer pin update. New bytes do not inherit device qualification automatically. There is no automatic re-pin or hash bypass. The publication helper requires target and packaged recipe inputs to match; this newly integrated helper must not be assumed to publish the retained package unchanged.

## Source recipe

[build-angle-apple.sh](../scripts/ios/build-angle-apple.sh) runs the retained [recipe](../scripts/ios/angle-recipe.py). [Pins](../scripts/ios/angle-pins.json) and [resolved dependencies](../scripts/ios/angle-resolved-deps.json) constrain upstream ANGLE, depot tools, acquired tool bytes, dependencies, arguments and patched source hashes. Both patches are required, in this order:

1. [RGB batch](../cmake/ios/patches/angle-rgb-batch.patch).
2. [RGB direct staging](../cmake/ios/patches/angle-rgb-direct-staging.patch).

The recipe explicitly lists its own inputs; application consumer pins are excluded so updating a consumer does not recursively change the dependency recipe identity. This integration changes the helper's hash, so future recipe identities differ from the retained tested package.

Source builds require native arm64 macOS 26.5.2 (25F84), Xcode 26.6 (17F113), iOS SDK 26.5 (23F81a), Python 3.14.6 and Apple Git 2.50.1 (Git-155). Preflight enforces these exact versions. Pinned depot tools acquire Clang `llvmorg-24-init-7747-g62397f8b-42`, GN and Ninja. A generic Xcode 26 installation or `macos-26` runner label alone does not establish compatibility.

The following commands are for a future source-build verification, not a way to recreate the current asset hashes:

```sh
bash scripts/ios/build-angle-apple.sh preflight
bash scripts/ios/build-angle-apple.sh sync --root "$PWD/build-angle-apple"
bash scripts/ios/build-angle-apple.sh verify-source --root "$PWD/build-angle-apple"
bash scripts/ios/build-angle-apple.sh build --root "$PWD/build-angle-apple" --variant qualified
bash scripts/ios/build-angle-apple.sh verify-package --package "$PWD/build-angle-apple/package-qualified"
```

`sync` requires new source/depot destinations; `build` refuses existing output/package directories. It creates a Metal-only, device arm64/iOS 17 package with system libc++, manual Objective-C reference counting, both patches, headers and licenses. No publication command is needed.

Source and toolchain pinning is not byte-for-byte package reproduction. ZIPs retain filesystem timestamps. Manifests include absolute paths, timings and audit-file hashes; recipe identity also includes tool paths. The tested package used an incremental build and a separately assembled manifest with empty timings. A normal clean recipe build records timings and full resolved provenance. It therefore cannot reproduce that manifest hash even if both stripped binaries happen to match. Clean binary equality itself is unverified. Do not spend a full dependency build expecting the current consumer pins to accept its output.

A retained dependency tree occupies about 8.2 GiB and the current package about 17 MiB. Reserve at least 15–20 GiB for a fresh source build and staging; this is a planning estimate. Earlier local compilation from already acquired source took about 52 seconds at `-j6`; fresh dependency acquisition was not measured and may dominate. Allow tens of minutes or longer for acquisition, depending on network and caches. The current app build tree is about 121 MiB, with separate source caches in addition. Prior app configure/build took about 149/53 seconds using local dependency sources; network and cache state affect fresh configure times.

## Observations and limits

The following are local observations through 2026-10-21, not a general device guarantee or completed P10 milestone:

- EAGL showed about 60 displayed frames/s on the tested iPhone 17 Pro Max / iOS 27.0.1 setup. EAGL also showed 120 on the tested 2018 and M4 iPad Pro setups. A universal phone/EAGL cap and its cause are unproven.
- Earlier short ANGLE windows showed MP 117.375 and SP 119.198 displayed frames/s. A later SP observation showed 119.467 frames/s over 3.834 seconds. These are earlier baselines, not sustained proof for the final RGB direct-staging candidate. Other trace exports remained UNKNOWN; prior sustained callback FAIL remains recorded.
- The final local RGB direct-staging candidate passed brief human SP/Hoth and MP/ffa3 controls, sound and visual checks. These do not establish objective sustained 120 fps or thermal behavior. Separate SP/MP saved settings were verified as ANGLE/120/Auto; code defaults were not changed.
- The 4 GB iPad memory observations in the player guide are historical floor proxies, not final ANGLE floor-memory qualification. A real 3 GB device remains untested.
- Sustained gameplay, thermal transitions, broader device/OS coverage, memory/drift, lifecycle and same-launch fallback qualification remain incomplete. Accepted save/load and scoped restart-memory evidence stands; missing resource measurements do not create a blanket new campaign.
- Display-link callbacks, engine FPS and Metal HUD counters are distinct from displayed frames. No battery-cost or full P10 completion claim follows from these results.

The player guide describes the current active-provider menu help. Provider selection still requires a full relaunch; changing the rate or restarting video does not switch providers.
