#!/usr/bin/env python3
"""Create the OpenJK AltStore/SideStore source JSON from a dist folder."""

import argparse
from datetime import datetime, timezone
import json
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dist", required=True)
    parser.add_argument("--version", required=True, help="CFBundleShortVersionString, e.g. 0.1.0")
    parser.add_argument("--build-version", help="CFBundleVersion; default: git rev-list --count HEAD")
    parser.add_argument("--repo", required=True)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()

    build_version = args.build_version or subprocess.check_output(
        ["git", "rev-list", "--count", "HEAD"], cwd=ROOT, text=True).strip()
    try:
        with open(os.path.join(args.dist, "SHA256SUMS"), encoding="utf-8") as sums:
            sha256 = dict(line.split()[::-1] for line in sums if line.strip())
    except OSError as error:
        print(f"error: cannot read SHA256SUMS: {error}", file=sys.stderr)
        return 1

    date = datetime.now(timezone.utc).isoformat(timespec="seconds")
    apps = []
    for flavor, description in (("SP", "Play the Jedi Academy story on iPhone and iPad."), ("MP", "Play Jedi Academy online on iPhone and iPad.")):
        filename = f"OpenJK-{flavor}-{args.version}.ipa"
        ipa = os.path.join(args.dist, filename)
        try:
            size = os.path.getsize(ipa)
        except OSError as error:
            print(f"error: cannot read {ipa}: {error}", file=sys.stderr)
            return 1
        if filename not in sha256:
            print(f"error: {filename} missing from SHA256SUMS", file=sys.stderr)
            return 1

        apps.append({
            "name": f"OpenJK {flavor}",
            "bundleIdentifier": f"org.openjk.ios.{flavor.lower()}",
            "developerName": "OpenJK contributors",
            "category": "games",
            "localizedDescription": (
                f"{description} Needs the game files from your own PC copy (Steam, GOG or disc): "
                "assets0.pk3 to assets3.pk3. Setup guide: "
                f"https://github.com/{args.repo}/blob/ios/docs/ios.md. "
                "Not affiliated with Disney, Lucasfilm or Raven Software."
            ),
            "iconURL": (
                f"https://raw.githubusercontent.com/{args.repo}/ios/"
                f"shared/icons/PNG/{flavor.lower()}512.png"
            ),
            "versions": [{
                "version": args.version,
                "buildVersion": build_version,
                "date": date,
                "downloadURL": (
                    f"https://github.com/{args.repo}/releases/download/"
                    f"ios-v{args.version}/{filename}"
                ),
                "size": size,
                "sha256": sha256[filename],
                "minOSVersion": "17.0",
            }],
            "appPermissions": {
                "entitlements": [],
                "privacy": {
                    "NSLocalNetworkUsageDescription": (
                        "Connects to multiplayer servers on your local network."
                    ),
                },
            },
        })

    source = {
        "name": "OpenJK",
        "identifier": "org.openjk.ios.source",
        "subtitle": "Open source Jedi Academy engine. Bring your own game files.",
        "website": f"https://github.com/{args.repo}",
        "apps": apps,
        "news": [{
            "title": f"OpenJK iOS {args.version}",
            "identifier": f"org.openjk.ios.release.{args.version}",
            "caption": "Release notes and setup guide on GitHub.",
            "date": date,
            "url": f"https://github.com/{args.repo}/releases/tag/ios-v{args.version}",
            "notify": True,
        }],
    }
    try:
        os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
        with open(args.out, "w", encoding="utf-8") as output:
            json.dump(source, output, indent=2)
            output.write("\n")
    except OSError as error:
        print(f"error: cannot write {args.out}: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
