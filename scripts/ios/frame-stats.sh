#!/usr/bin/env bash
# Summarises perf: and frameStats: lines after the last run-start marker. Needs gawk (brew install gawk).
set -eu
log="$1"
ts='^([0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2} )?'
run=$(gawk -v ts="$ts" '
	$0 ~ (ts "frameStats: run-start") { n = 0; found = 1; next }
	found && $0 ~ (ts "(perf|frameStats):") { sub(ts, ""); line[++n] = $0 }
	END { if (!found) exit 1; for (i = 1; i <= n; i++) print line[i] }' "$log") || {
	echo "no 'frameStats: run-start' line: run ios_frameStatsReset after the map loads" >&2
	exit 1
}
perf=$(printf '%s\n' "$run" | grep '^perf:' || true)
if [ -n "$perf" ]; then
	printf '%s\n' "$perf" | gawk '{
		for (i = 2; i <= NF; i++) { split($i, kv, "="); v[kv[1]] = kv[2] + 0 }
		a += v["backend_avg"]; if (v["backend_p95"] > m) m = v["backend_p95"]
		pa += v["present_avg"]; if (v["present_p95"] > pm) pm = v["present_p95"]
		b += v["batches"]; d += v["draws_est"]; t += v["tris"]; n++
	} END {
		printf "backend: windows=%d mean_avg=%.2f ms worst_p95=%.2f ms present_avg=%.2f ms worst_present_p95=%.2f ms batches=%.0f draws_est=%.0f tris=%.0f\n",
			n, a/n, m, pa/n, pm, b/n, d/n, t/n
	}'
else
	echo "backend: no perf: lines (build without OPENJK_IOS_PERF)"
fi
# "frameStats: load" lines are stalls, not windows
frames=$(printf '%s\n' "$run" | grep '^frameStats: n=' || true)
if [ -n "$frames" ]; then
	printf '%s\n' "$frames" | gawk '{
		for (i = 2; i <= NF; i++) { split($i, kv, "="); v[kv[1]] = kv[2] + 0 }
		n++; t = v["target"]; p95[n] = v["p95"]; c50[n] = v["cpu_p50"]; c95[n] = v["cpu_p95"]
		np50[n] = v["cpu_nopresent_p50"]; np95[n] = v["cpu_nopresent_p95"]; pr95[n] = v["present_p95"]
		le9 += v["le9"]; le17 += v["le17"]; le34 += v["le34"]; gt34 += v["gt34"]
	} END {
		asort(p95); asort(c50); asort(c95); asort(np50); asort(np95); asort(pr95)
		tot = le9 + le17 + le34 + gt34; m = int((n + 1) / 2)
		printf "frames: windows=%d target=%d median_window_p95=%.2f ms le9=%.1f%% le17=%.1f%% le34=%.1f%% gt34=%.1f%% gt17=%.2f%% cpu_p50=%.2f ms cpu_p95=%.2f ms cpu_nopresent_p50=%.2f ms cpu_nopresent_p95=%.2f ms present_p95=%.2f ms\n",
			n, t, p95[m], 100*le9/tot, 100*le17/tot, 100*le34/tot, 100*gt34/tot, 100*(le34+gt34)/tot, c50[m], c95[m], np50[m], np95[m], pr95[m]
	}'
	printf '%s\n' "$frames" | tail -1 | sed 's/.*\(run_n=.*\)/run: \1/'
else
	echo "frames: no frameStats: lines (set ios_frameStats 1)"
fi
# run_loads comes from the last complete window; stalls after it are only in the load lines
printf '%s\n' "$run" | gawk '
	/^frameStats: load / { n++; t++; ms = $3 + 0; if (ms > max) max = ms; list = list " " ms }
	/^frameStats: n=/ { t = 0 }
	END {
		printf "loads: after_marker=%d max_ms=%.0f trailing=%d%s\n", n, max, t, n ? " ms:" list : ""
		if (t) printf "WARNING: %d stall(s) after the last complete window; not in run_loads\n", t
	}'
