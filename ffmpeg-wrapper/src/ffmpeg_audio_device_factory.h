/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree.
 * 
 *  Referenced from modules/audio_device/dummy/file_audio_device_factory.h
 */

#ifndef AUDIO_DEVICE_FFMPEG_AUDIO_DEVICE_FACTORY_H_
#define AUDIO_DEVICE_FFMPEG_AUDIO_DEVICE_FACTORY_H_

#include <stdint.h>

class FFmpegAudioDevice;

class FFmpegAudioDeviceFactory {
 public:
  static FFmpegAudioDevice* CreateFFmpegAudioDevice();
};

#endif  // AUDIO_DEVICE_FFMPEG_AUDIO_DEVICE_FACTORY_H_
