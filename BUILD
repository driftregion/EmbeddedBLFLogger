package(default_visibility = ["//visibility:public"])


cc_library(
    name="miniz",
    srcs = [
        "miniz/miniz.c",
        "miniz/miniz.h",
    ]
)

cc_library(
    name="blflogger",
    srcs = [
        "blflogger.cpp",
        "blflogger.h",
    ],
    deps=[":miniz"],
)

cc_library(
    name = "can-utils",
    srcs = [
        "can-utils/lib.c",
        "can-utils/lib.h",
        "can-utils/include/linux/can.h",
    ],
    copts = [
        "-Ican-utils/include",
    ],
)

cc_binary(
    name="log2blf",
    srcs=[
        "log2blf.cpp",
    ],
    deps=[
        ":can-utils",
        ":blflogger",
    ]
)
