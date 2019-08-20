/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree.
 * 
 *  Referenced from modules/audio_device/dummy/file_audio_device_factory.cc
 */

#include "ffmpeg_audio_device_factory.h"
#include "ffmpeg_audio_device.h"

FFmpegAudioDevice* FFmpegAudioDeviceFactory::CreateFFmpegAudioDevice()
{ return new FFmpegAudioDevice(); }
