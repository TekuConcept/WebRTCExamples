# FFmpeg <=> WebRTC
Example code demonstrating how to transcode between FFmpeg and WebRTC.


The code can be extended and optimized to use native ffmpeg to synchronize video and audio streams. This example, for simplicity, only uses ffmpeg CLI tools and pipes to transcode media.

- - -

## Getting Started

```
# Create and enter our working directory
mkdir webrtc
cd webrtc

# Get Depot Tools
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=$PATH:/path/to/depot_tools

# For Windows or for more information, see
# https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html

# Get WebRTC
# Note: This step may take an hour or two.
fetch --nohooks webrtc
cd src
git checkout branch-heads/4434
gclient sync
# for branch-heads/72, checkout the code a commit or two back


# Build WebRTC
gn gen out/Default   # generate ninja build scripts
ninja -C out/Default # change to (-C) out/Default and build

# To add command-line build options:
# > gn gen out/Default "--args=<your args>"

# To build a single target instead of all targets:
# > ninja -C out/Default <target>
```

## Copy Files and Apply Patch

```
cd webrtc # go back to the working directory
git clone https://github.com/TekuConcept/FFmpeg_WebRTC.git

# apply patch file
cd src
git am --signoff < ../FFmpeg_WebRTC/0001-FFmpeg-Adapter.patch

# copy ffmpeg adapter files to example folder
TARGET_DIR=examples/peerconnection/client/ffmpeg
mkdir $TARGET_DIR
cp ../FFmpeg_WebRTC/src/* $TARGET_DIR

# (re)run the webrtc build
gn gen out/Default
ninja -C out/Default
```

## Making Changes

The specific source files that handle video and audio input are `ffmpeg_video_capture_module` and `ffmpeg_audio_device` respectively. While ffmpeg audio and video output are not yet handled in this example, they can easily be introduced. (See `_outputFile` in `ffmpeg_audio_device` for audio output; See `VideoRenderer` in `linux/main_wnd.cc` for video output.)

Hardware encoding and decoding pass-through logic may be implemented here in `conductor.cc`...

```
peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
      nullptr, worker_thread_.get(), nullptr, default_adm,
      webrtc::CreateBuiltinAudioEncoderFactory(), // <--
      webrtc::CreateBuiltinAudioDecoderFactory(), // <--
      webrtc::CreateBuiltinVideoEncoderFactory(), // <--
      webrtc::CreateBuiltinVideoDecoderFactory(), // <-- These lines
      nullptr, nullptr);
```

## Remarks

Given that ffmpeg is used to send raw media to WebRTC, this opens up more possibilities with WebRTC such as being able live-stream IP cameras that use browser-incompatible protocols (like RTSP) or pre-recorded video simulations.
