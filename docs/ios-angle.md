# ANGLE dependency

- ANGLE over Metal is an opt-in GLES provider. EAGL stays the default. See [Choose a provider](ios.md#choose-a-provider).
- Both apps always embed ANGLE, even when EAGL runs.

## Delivery

- [angle-consumer.json](../scripts/ios/angle-consumer.json) pins four assets: `manifest.json`, `SHA256SUMS`, `libEGL.xcframework.zip`, `libGLESv2.xcframework.zip`.
- At configure, [angle-consumer.py](../scripts/ios/angle-consumer.py) downloads them over HTTPS and checks every hash, the manifest identity and both framework binaries.
- No ANGLE source build is needed. Run `scripts/build-ios.sh` as usual.
- Offline: set `OPENJK_IOS_ANGLE_LOCAL_ASSETS=/absolute/path/to/package` to a folder with the same four files. The same checks apply.

```sh
OPENJK_IOS_ANGLE_LOCAL_ASSETS=/absolute/path/to/package scripts/build-ios.sh
```

## Patches in the binary

The package is upstream ANGLE plus two patches, applied in order:

1. [RGB batch](../cmake/ios/patches/angle-rgb-batch.patch): converts a whole RGB8 2D upload to RGBA8 in one batch (up to 1 MiB).
2. [RGB direct staging](../cmake/ios/patches/angle-rgb-direct-staging.patch): converts that batch straight into a Metal staging buffer when the texture uses staged uploads.

## Source and provenance

- Release: [`angle-3c3c1746733c-rgb-playable-direct-r1-415d50aa73e26917-b386b9e0a9eb3237-ios-arm64`](https://github.com/Nodman/OpenJK-iOS/releases/tag/angle-3c3c1746733c-rgb-playable-direct-r1-415d50aa73e26917-b386b9e0a9eb3237-ios-arm64).
- Provenance commit: [`f5e36d8187999560049cdd42b344ad3f2c2d5db1`](https://github.com/Nodman/OpenJK-iOS/commit/f5e36d8187999560049cdd42b344ad3f2c2d5db1). The full build recipe lives there.
- A new package needs a new release and a deliberate update of `angle-consumer.json`.
