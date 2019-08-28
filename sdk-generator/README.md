# WebRTC SDK Generator
Turn a pre-compiled native-WebRTC project into a platform-specific SDK.

The ninja build system is great for the WebRTC project itself, but when it comes to building new native projects using the new libraries, the build-system starts feeling a little crouded, limited, and clunky.

This bash script extracts all the headers, libraries, and object files needed to build new projects. The script also generates a CMake file that can then be included in any CMake project.

_Note: The script was specifically written for and tested on Debian-AMD64-Linux systems. An ARM64 build is perhaps the next target to look into._

The SDK output folder structure will look as follows:
```
- webrtc_sdk
    - include
    - lib
    - sysroot
        - usr
            - include
            - lib
    - WebRTCConfigure.cmake
```

## Getting Started

Fetch the native-WebRTC source code, then build it using the following commands:
```
# Note: tested with git branch "branch-heads/72"
gn gen out/GCC --args="\
    is_clang=false \
    is_debug=false \
    is_component_build=false \
    use_custom_libcxx=false \
    use_cxx11=true \
    rtc_include_tests=false \
    rtc_build_examples=true \
    treat_warnings_as_errors=false"
ninja -C out/GCC
```

Run the generator:
```
./generate_webrtc_sdk.sh \
    -o /path/to/webrtc_sdk \
    -w /path/to/webrtc/src \
    /path/to/webrtc/src/out/GCC
```

## CMake

`WebRTCConfigure.cmake` is generated as part of the SDK. It can be added to any CMake project as follows:
```
# CMakeLists.txt

...

# WEBRTC_SDK_DIR must be set before including the cmake file
# or the file will not configure properly and build-errors
# will ensue.
SET(WEBRTC_SDK_DIR /path/to/webrtc_sdk)
INCLUDE(${WEBRTC_SDK_DIR}/WebRTCConfigure.cmake)

...
```
