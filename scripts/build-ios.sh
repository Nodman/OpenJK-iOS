#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"
version="${OPENJK_IOS_VERSION:-0.1.0}"
sdk_major="$(xcrun --sdk iphoneos --show-sdk-version | cut -d. -f1)"
if [ "$sdk_major" -ge 27 ]; then
	echo "error: iOS SDK $sdk_major found. SDL2 needs the UIScene patch (plan P8) for SDK 27+."
	echo "Use Xcode 26.x: export DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer (an Xcode 26.x install)"
	exit 1
fi
# always pass the team id: the cache would keep an old signed configure otherwise
cmake --preset ios -DOPENJK_IOS_ANGLE_LOCAL_ASSETS="${OPENJK_IOS_ANGLE_LOCAL_ASSETS:-}" -DOPENJK_IOS_VERSION="$version" -DOPENJK_IOS_TEAM_ID="${OPENJK_IOS_TEAM_ID:-}"
cmake --build --preset ios-release --target openjk.arm64 openjk_sp.arm64
check_args=(--prepared build-ios/angle-consumer)
[ -z "${OPENJK_IOS_TEAM_ID:-}" ] || check_args+=(--signed)
dist="build-ios/dist"
rm -rf "$dist" && mkdir -p "$dist"
for flavor in MP SP; do
	app="build-ios/apps/Release-iphoneos/OpenJK-$flavor.app"
	[ -d "$app" ] || { echo "error: $app missing"; exit 1; }
	stage="$(mktemp -d)"
	mkdir "$stage/Payload"
	cp -R "$app" "$stage/Payload/"
	python3 scripts/ios/angle-consumer.py check-app --app "$stage/Payload/OpenJK-$flavor.app" "${check_args[@]}"
	(cd "$stage" && zip -qry "$root/$dist/OpenJK-$flavor-$version.ipa" Payload)
	rm -rf "$stage"
done
(cd "$dist" && shasum -a 256 -- *.ipa > SHA256SUMS)
ls -l "$dist"
