solutions = [{
  "name": ".",
  "url": "https://chromium.googlesource.com/angle/angle.git",
  "managed": False,
  "deps_file": "DEPS",
  "custom_vars": {
    "checkout_angle_cl_deps": False,
    "checkout_angle_dawn_deps": False,
    "checkout_angle_internal": False,
    "checkout_angle_restricted_traces": False,
    "checkout_extra_traces": False,
    "checkout_angle_mesa": False,
    "checkout_angle_partition_alloc": False,
    "checkout_clang_tidy": False,
    "checkout_clang_coverage_tools": False,
    "download_remoteexec_cfg": False,
  },
}]
target_os = ["ios"]
