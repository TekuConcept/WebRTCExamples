#!/bin/bash

#
# Created by TekuConcept on August 22, 2019
#
# Generate webrtc native build project with
# > gn gen out/GCC --args="is_clang=false is_debug=false \
# > is_component_build=false use_custom_libcxx=false \
# > use_cxx11=true rtc_include_tests=false \
# > rtc_build_examples=true treat_warnings_as_errors=false"
#

print_usage_and_die() {
    echo "Usage: $0 [-o <output_dir>] [-w <webrtc_dir>] build_dir"
    echo "Options:"
    echo "    --clear:      deletes the output directory without"
    echo "                  asking if it already exists"
    echo "    --no-sysroot: do not copy all third-party shared libraries"
    echo "                  from the webrtc virtual sysroot to the sdk"
    exit 1
}

assert_directory() {
    if [ ! -d "$1" ]; then
        echo "Error: $1 is not a directory or does not exist"
        print_usage_and_die
    fi
}

assert_file_exists() {
    if [ ! -f "$1" ]; then
        echo "Error: Cannot find file $1"
        exit 1
    fi
}

assert_directory_exists() {
    if [ ! -d "$1" ]; then
        echo "Error: Cannot find directory $1"
        exit 1
    fi
}

assert_file_or_dir_exists() {
    if [ ! -d "$1" ] && [ ! -f "$1" ]; then
        echo "Error: Cannot find path $1"
        exit 1
    fi
}



#
# Parse command line arguments
#

EXEC_DIR="$(readlink -f "$( dirname "${BASH_SOURCE[0]}")")"

# enumerate through all args
# separate key:value pairs from tokens
TOKENS=()
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        -o|--output-dir)
            OUTPUT_DIR="$2"
            shift
            ;;
        -w|--webrtc-dir)
            WEBRTC_DIR="$2"
            shift
            ;;
        *)    # unknown option
            TOKENS+=("$1")
            ;;
    esac
    shift # past argument
done

# parse non-key:value tokens
CLEAR_OUTPUT=false
WITH_SYSROOT=true
if [[ ${#TOKENS[@]} < 1 ]]; then print_usage_and_die; fi
for token in "${TOKENS[@]}"; do
    case $token in
        --clear)
            CLEAR_OUTPUT=true
            ;;
        --no-sysroot)
            WITH_SYSROOT=false
            ;;
        *)
            # TODO: check if too many wildcards
            BUILD_DIR=$token
            ;;
    esac
done



#
# Interpret arguments
#

if [ -z "$BUILD_DIR" ]; then
    echo "build directory not specified"
    print_usage_and_die
fi
assert_directory $BUILD_DIR
BUILD_DIR=$(readlink -f $BUILD_DIR)

if [ -z "$WEBRTC_DIR" ]; then
    WEBRTC_DIR=$BUILD_DIR/../..
fi
assert_directory $WEBRTC_DIR
WEBRTC_DIR=$(readlink -f $WEBRTC_DIR)

if [ -z "$OUTPUT_DIR" ]; then
    OUTPUT_DIR=$(readlink -f .)/webrtc_sdk
else
    if [ ! -d "$OUTPUT_DIR" ]; then mkdir -p $OUTPUT_DIR; fi
    OUTPUT_DIR=$(readlink -f $OUTPUT_DIR)
fi

echo "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -"
echo "Output: $(realpath --relative-to="$EXEC_DIR" "$OUTPUT_DIR")"
echo "WebRTC: $(realpath --relative-to="$EXEC_DIR" "$WEBRTC_DIR")"
echo "Build:  $(realpath --relative-to="$EXEC_DIR" "$BUILD_DIR")"
echo "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -"

OUTPUT_LIB_DIR=$OUTPUT_DIR/lib
OUTPUT_INCLUDE_DIR=$OUTPUT_DIR/include
OUTPUT_TEMP_DIR=$OUTPUT_DIR/temp

if [ "$CLEAR_OUTPUT" = true ]; then
    rm -rf $OUTPUT_DIR # reset for next generation
elif [ "$(ls -A $OUTPUT_DIR)" ]; then
    echo "$OUTPUT_DIR is not empty"
    echo "You can skip this check by adding --clear"
    read -p "Continue? (Y/N): " confirm && \
        [[ $confirm == [yY] || $confirm == [yY][eE][sS] ]] || \
        exit 1
    rm -rf $OUTPUT_DIR
fi
mkdir -p $OUTPUT_LIB_DIR
mkdir -p $OUTPUT_INCLUDE_DIR



#
# Copy libraries and objects files
#

# Some archives were not included in the webrtc archive.
# To keep things all self-contained for projects linking
# against webrtc, we combine the archives into one.
BUILD_OBJ_DIR=$BUILD_DIR/obj

archive_files=(
    $BUILD_OBJ_DIR/libwebrtc.a \
    $BUILD_OBJ_DIR/rtc_base/librtc_base.a \
)

object_folders=(\
    $BUILD_OBJ_DIR/rtc_base/rtc_base/ \
    $BUILD_OBJ_DIR/rtc_base/rtc_json/ \
    $BUILD_OBJ_DIR/third_party/jsoncpp/jsoncpp/ \
    $BUILD_OBJ_DIR/test/field_trial/ \
)

for token in "${archive_files[@]}"; do
    assert_file_exists $token
done

for token in "${object_folders}"; do
    assert_directory_exists $token
done

cd $BUILD_OBJ_DIR
for folder in "${object_folders[@]}"; do
    find $folder -type f -name \*.o | while read FILE; do
        TARGET=$(realpath --relative-to="$BUILD_OBJ_DIR" "$FILE")
        echo "Copying ${OUTPUT_LIB_DIR}/${TARGET}"
        cp --parents "$TARGET" -t $OUTPUT_LIB_DIR
    done
done

for archive in "${archive_files[@]}"; do
    TARGET=$(realpath --relative-to="$BUILD_OBJ_DIR" "$archive")
    echo "Copying ${OUTPUT_LIB_DIR}/${TARGET}"
    cp --parents "$TARGET" -t $OUTPUT_LIB_DIR
done
cd $EXEC_DIR



#
# Copy include files
#

# Not all folders contain headers we want to copy,
# so only white-list the folders we do want to copy.
include_folders=(\
    "api" \
    "audio" \
    "base" \
    "call" \
    "common_audio" \
    "common_video" \
    "logging" \
    "media" \
    "modules" \
    "p2p" \
    "pc" \
    "rtc_base" \
    "rtc_tools" \
    "stats" \
    "system_wrappers" \
    "test" \
    "video" \
)

cd $WEBRTC_DIR # enter source directory to capture file-directory structure
for folder in "${include_folders[@]}"; do
    find $WEBRTC_DIR/$folder -type f -name \*.h | while read FILE; do
        TARGET=$(realpath --relative-to="$WEBRTC_DIR" "$FILE")
        echo "Copying ${OUTPUT_INCLUDE_DIR}/${TARGET}"
        cp --parents "$TARGET" -t $OUTPUT_INCLUDE_DIR
    done
done
cd $EXEC_DIR # go back to executing directory

echo "Copying ${OUTPUT_INCLUDE_DIR}/common_types.h"
cp $WEBRTC_DIR/common_types.h $OUTPUT_INCLUDE_DIR/common_types.h



#
# Copy sysroot (currently only selects x86_64-linux;debian-sid)
#

if [ "$WITH_SYSROOT" = true ]; then
    OUTPUT_SYSROOT_DIR=$OUTPUT_DIR/sysroot
    OUTPUT_SYSROOT_INCLUDE_DIR=${OUTPUT_SYSROOT_DIR}/usr/include
    OUTPUT_SYSROOT_LIB_DIR=${OUTPUT_SYSROOT_DIR}/usr/lib
    mkdir -p ${OUTPUT_SYSROOT_INCLUDE_DIR}
    mkdir -p ${OUTPUT_SYSROOT_LIB_DIR}

    SYSROOT_DIR=$WEBRTC_DIR/build/linux/debian_sid_amd64-sysroot
    SYSROOT_INCLUDE_DIR=$SYSROOT_DIR/usr/include
    SYSROOT_LIB_DIR=$SYSROOT_DIR/usr/lib/x86_64-linux-gnu

    assert_directory_exists $SYSROOT_DIR
    assert_directory_exists $SYSROOT_INCLUDE_DIR
    assert_directory_exists $SYSROOT_LIB_DIR

    third_party_includes=(\
        "${SYSROOT_INCLUDE_DIR}/atk-1.0/." \
        "${SYSROOT_INCLUDE_DIR}/cairo/." \
        "${SYSROOT_INCLUDE_DIR}/gdk-pixbuf-2.0/." \
        "${SYSROOT_INCLUDE_DIR}/glib-2.0/." \
        "${SYSROOT_INCLUDE_DIR}/gtk-3.0/." \
        "${SYSROOT_INCLUDE_DIR}/jsoncpp/." \
        "${SYSROOT_INCLUDE_DIR}/pango-1.0/." \
        "${SYSROOT_LIB_DIR}/glib-2.0/include/glibconfig.h" \
        "${WEBRTC_DIR}/third_party/libyuv/include/libyuv" \
        "${WEBRTC_DIR}/third_party/libyuv/include/libyuv.h" \
        "${WEBRTC_DIR}/third_party/abseil-cpp/absl" \
    )

    for token in "${third_party_includes[@]}"; do
        assert_file_or_dir_exists $token
        echo "Copying $(basename $token)"
        cp -ar $token -t $OUTPUT_SYSROOT_INCLUDE_DIR
    done

    # cleanup absl folder
    # NOTE: this takes time (~250 ms)
    find $OUTPUT_SYSROOT_INCLUDE_DIR/absl ! -name \*.h -type f | \
    while read FILE; do rm $FILE; done

    find $SYSROOT_LIB_DIR -maxdepth 1 -type f -name \*.so\* | \
    while read FILE; do
        echo "Copying $(basename $FILE)"
        cp $FILE -t $OUTPUT_SYSROOT_LIB_DIR
    done

    find $SYSROOT_LIB_DIR -maxdepth 1 -type l -name \*.so\* | \
    while read FILE; do
        # copy base file
        base_file=$(readlink -f $FILE)
        echo "Copying $(basename $base_file)"
        cp $base_file -t $OUTPUT_SYSROOT_LIB_DIR

        # echo "- - - - -"
        # echo "Symlink: $FILE"
        # echo "File:    $base_file"

        # rebuild symlinks
        loop_continue=true
        while [ "$loop_continue" = true ]; do
            reflink=$FILE
            parent=""
            next=$OUTPUT_SYSROOT_LIB_DIR/$(basename $reflink)
            while [ ! -L $next ] && [ ! -f $next ]; do
                # next link assumed to be relative
                # TODO: maybe save relative path to resolve next link
                reflink=${SYSROOT_LIB_DIR}/$(readlink $reflink)
                parent=$next
                next=$OUTPUT_SYSROOT_LIB_DIR/$(basename $reflink)
            done
            if [ -z "$parent" ]; then
                loop_continue=false;
            else
                # we use basename because the symlink will exist
                # in the same directory as the target
                echo "Symlink $(basename $parent)"
                ln -s $(basename $next) $parent
            fi
        done
    done

    SYSROOT_LIB_DIR=$(realpath --relative-to="$OUTPUT_DIR" "$OUTPUT_SYSROOT_LIB_DIR")
    SYSROOT_LIB_DIR="\${WEBRTC_SDK_DIR}/${SYSROOT_LIB_DIR}"
    SYSROOT_INCLUDE_DIR=$(realpath --relative-to="$OUTPUT_DIR" "$OUTPUT_SYSROOT_INCLUDE_DIR")
    SYSROOT_INCLUDE_DIR="\${WEBRTC_SDK_DIR}/${SYSROOT_INCLUDE_DIR}"
else
    SYSROOT_LIB_DIR=/usr/lib
    SYSROOT_INCLUDE_DIR=/usr/include
fi



#
# Generate cmake file
#

CMAKE_FILE=$OUTPUT_DIR/WebRTCConfigure.cmake
NINJA_BUILD_REFERENCE=$BUILD_OBJ_DIR/examples/peerconnection_client.ninja
assert_file_exists $NINJA_BUILD_REFERENCE
DEFINES=$(head -n 1 $NINJA_BUILD_REFERENCE)
DEFINES=$(echo ${DEFINES:10}) # clip first 10 chars
DEFINES=$(echo $DEFINES | sed 's/ /\n/g')

# Note: C and C++ flags can also be found in ${NINJA_BUILD_REFERENCE}
#       but for now they're hard coded here.

printf "\
#
# Sets up variables needed to compile against the WebRTC library
#
# NOTE: set WEBRTC_SDK_DIR to the path of the generated
#       sdk folder before including this file
#

SET(WEBRTC_INCLUDE_DIR
    \${WEBRTC_SDK_DIR}/include
    ${SYSROOT_INCLUDE_DIR}
)

SET(WEBRTC_LIB_DIR \${CMAKE_SOURCE_DIR}/Libraries/webrtc_sdk/lib)
FILE(GLOB_RECURSE WEBRTC_LIB \${WEBRTC_LIB_DIR}/*.a)
FILE(GLOB_RECURSE WEBRTC_DEPENDENCIES
    \${WEBRTC_LIB_DIR}/rtc_base/json.o
    \${WEBRTC_LIB_DIR}/third_party/jsoncpp/json_reader.o
    \${WEBRTC_LIB_DIR}/third_party/jsoncpp/json_writer.o
    \${WEBRTC_LIB_DIR}/third_party/jsoncpp/json_value.o
    \${WEBRTC_LIB_DIR}/test/field_trial.o
)

SET(WEBRTC_LIBS
    X11 Xcomposite Xext Xrender atomic dl pthread rt
    gmodule-2.0 gtk-3 gdk-3 pangocairo-1.0 pango-1.0
    atk-1.0 cairo-gobject cairo gdk_pixbuf-2.0 gio-2.0
    gobject-2.0 gthread-2.0 glib-2.0 m jsoncpp

    \${WEBRTC_LIB}
    \${WEBRTC_DEPENDENCIES}
)

SET(WEBRTC_CXX_FLAGS \"\\
    -Wno-deprecated-declarations \
    -fno-strict-aliasing \
    --param=ssp-buffer-size=4 \
    -fstack-protector \
    -Wno-builtin-macro-redefined \
    -funwind-tables \
    -fPIC \
    -pipe \
    -pthread \
    -m64 \
    -march=x86-64 \
    -Wall \
    -Wno-unused-local-typedefs \
    -Wno-deprecated-declarations \
    -Wno-comments \
    -Wno-missing-field-initializers \
    -Wno-unused-parameter \
    -fno-ident \
    -fdata-sections \
    -ffunction-sections \
    -fno-omit-frame-pointer \
    -fvisibility=hidden \
    -Wextra \
    -Wno-unused-parameter \
    -Wno-missing-field-initializers \
    -Wno-narrowing \
    -fno-exceptions \
    -fno-rtti \
    -fvisibility-inlines-hidden \
    -Wnon-virtual-dtor \
\")

SET(WEBRTC_LINK_OPTIONS \"\\
    -Wl,--fatal-warnings \\
    -fPIC \\
    -Wl,-z,noexecstack \\
    -Wl,-z,now \\
    -Wl,-z,relro \\
    -Wl,-z,defs \\
    -Wl,--as-needed \\
    -fuse-ld=gold \\
    -Wl,--threads \\
    -Wl,--thread-count=4 \\
    -Wl,--icf=all \\
    -m64 \\
    -Wl,-O2 \\
    -Wl,--gc-sections \\
    -L${SYSROOT_LIB_DIR} \\
    -Wl,-rpath-link=${SYSROOT_LIB_DIR} \\
    -pie \\
    -Wl,-rpath-link=. \\
    -Wl,--disable-new-dtags \\
\")

SET(WEBRTC_COMPILE_DEFINITIONS ${DEFINES} )
" > $CMAKE_FILE



#
# Finalize
#

echo -e "\033[0;92m -- FINISHED --\033[0m"
