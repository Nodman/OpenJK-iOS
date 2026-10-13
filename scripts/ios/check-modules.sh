#!/usr/bin/env bash
# Module exports must match the allowlist; the renderer must not bind native GLES.
# v1 checks the embedded dylibs per app. Pass "static" as the 3rd arg for P9 builds only.
set -eu
export LC_ALL=C
build="${1:?build dir}"
set_name="${2:?mp or all}"
mode="${3:-dylib}"
apps="$build/apps/Release-iphoneos"
mods="$build/modules/Release-iphoneos"
fail=0
check() {
	local app="$1" t="$2"; shift 2
	local dyn=() stat=() side=dyn a
	for a in "$@"; do
		[ "$a" = "--" ] && { side=stat; continue; }
		if [ "$side" = dyn ]; then dyn+=("$a"); else stat+=("$a"); fi
	done
	local lib want
	if [ "$mode" = static ]; then
		[ -f "$mods/$t.a" ] || { echo "FAIL $t: static archive missing"; fail=1; return; }
		lib="$mods/$t.a"; want=$(printf '%s\n' ${stat[@]+"${stat[@]}"} | sort | tr '\n' ' ')
	else
		[ -f "$apps/OpenJK-$app.app/Frameworks/$t.dylib" ] || { echo "FAIL $t missing in OpenJK-$app.app/Frameworks"; fail=1; return; }
		lib="$apps/OpenJK-$app.app/Frameworks/$t.dylib"; want=$(printf '%s\n' ${dyn[@]+"${dyn[@]}"} | sort | tr '\n' ' ')
	fi
	local got; got=$(nm -gU "$lib" | awk 'NF==3 {print $3}' | sort | tr '\n' ' ')
	if [ "$want" != "$got" ]; then echo "FAIL $t: want [$want] got [$got]"; fail=1; else echo "ok   $t ($lib)"; fi
	case "$t" in rd*-vanilla_arm64)
		local n
		n=$(nm -u "$lib" | grep -c '^_gl[A-Z]' || true)
		if [ "$n" = 0 ]; then echo "ok   $t binds no native _gl* symbols"; else echo "FAIL $t binds $n native _gl* symbols"; fail=1; fi
		n=$(nm -gU "$lib" | grep -c gl4es || true)
		if [ "$n" = 0 ]; then echo "ok   $t exports no gl4es symbols"; else echo "FAIL $t exports $n gl4es symbols"; fail=1; fi ;;
	esac
}
check MP jampgamearm64 _GetModuleAPI _dllEntry _vmMain -- _jampgame_GetModuleAPI
check MP cgamearm64 _GetModuleAPI _dllEntry _vmMain -- _cgame_GetModuleAPI
check MP uiarm64 _GetModuleAPI _dllEntry _vmMain -- _ui_GetModuleAPI
check MP rd-vanilla_arm64 _GetRefAPI -- _rdvanilla_GetRefAPI
if [ "$set_name" = "all" ]; then
	check SP jagamearm64 _GetGameAPI _dllEntry _vmMain -- _jagame_GetGameAPI _jagame_dllEntry _jagame_vmMain
	check SP rdsp-vanilla_arm64 _GetRefAPI -- _rdspvanilla_GetRefAPI
fi
gl4es="$build/gl4es/Release/libGL.a"
if [ -f "$gl4es" ]; then
	undefined=$(nm -u "$gl4es")
	defined=$(nm -gU "$gl4es")
	native=$(comm -23 \
		<(printf '%s\n' "$undefined" | awk '/^_gl[A-Z]/ {print $1}' | sort -u) \
		<(printf '%s\n' "$defined" | awk 'NF==3 {print $3}' | sort -u))
	if [ -z "$native" ]; then
		echo "ok   gl4es archive binds no native _gl* symbols ($gl4es)"
	else
		echo "FAIL gl4es archive binds native _gl* symbols: $native"
		fail=1
	fi
else
	echo "FAIL gl4es archive missing: $gl4es"
	fail=1
fi
exit "$fail"
