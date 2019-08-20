/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree.
 * 
 *  Referenced from modules/video_capture/video_capture_factory.cc
 */

#include "ffmpeg_video_factory.h"
#include "ffmpeg_video_capture_module.h"


FFmpegVideoFactory::FFmpegVideoFactory() { }


FFmpegVideoFactory::~FFmpegVideoFactory() { }


rtc::scoped_refptr<webrtc::VideoCaptureModule>
FFmpegVideoFactory::Create(const char* device_id) {
    if (device_id == nullptr) return nullptr;
    rtc::scoped_refptr<FFmpegVideoCaptureModule> capture(
        new rtc::RefCountedObject<FFmpegVideoCaptureModule>(
            std::string(device_id)));
    return capture;
}


webrtc::VideoCaptureModule::DeviceInfo*
FFmpegVideoFactory::CreateDeviceInfo()
{ return FFmpegVideoCaptureModule::CreateDeviceInfo(); }


void
FFmpegVideoFactory::DestroyDeviceInfo(
    webrtc::VideoCaptureModule::DeviceInfo* info)
{ delete info; }
