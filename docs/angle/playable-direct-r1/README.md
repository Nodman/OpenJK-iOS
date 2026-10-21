# Retained ANGLE playable dependency

This provenance commit records the original 11 recipe inputs for the retained tested incremental package. It is a dependency hosting record, not an app integration commit or a clean full rebuild.

The assembly independently copied the qualified source/toolchain, applied RGB batch then production direct staging, and reused 472 of 474 compiled objects. The two changed objects were TextureMtl and the generated version object. The original recipe, ordered patches, object comparison, copy proof and assembly commands are retained here. `provenance.json` records the exact assets, source, recipe and payload identities.

The four published files are libEGL.xcframework.zip, libGLESv2.xcframework.zip, SHA256SUMS and manifest.json. Existing license trees stay inside both framework ZIPs. None of these files is rebuilt, repacked or rewritten for publication. Future normal recipe builds may produce different envelope hashes because of timings, paths and ZIP timestamps.

The package production variant is named `qualified`; this does not mean full P10 qualification. Brief human SP/MP playtests were accepted; sustained performance, thermal and broader device qualification remain open. No app release or default change is part of this dependency publication.

The original helper discovers recipe files with a wildcard. This checkout deliberately contains no later app consumer files. Its discovered inputs must equal the retained manifest, and the target commit must contain their exact blobs. Desktop release jobs exclude angle- tags; no dependency workflow dispatch is used.
