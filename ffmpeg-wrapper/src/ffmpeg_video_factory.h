/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree.
 * 
 *  Referenced from modules/video_capture/video_capture_factory.h
 */

#ifndef DEMO_FFMPEG_VIDEO_FACTORY_H_
#define DEMO_FFMPEG_VIDEO_FACTORY_H_

// #include "media/engine/webrtcvideocapturer.h"
#include "modules/video_capture/video_capture.h"

class FFmpegVideoFactory
// : public cricket::WebRtcVcmFactoryInterface
{
public:
    FFmpegVideoFactory();
    ~FFmpegVideoFactory();
    static rtc::scoped_refptr<webrtc::VideoCaptureModule> Create(const char* device);
    static webrtc::VideoCaptureModule::DeviceInfo* CreateDeviceInfo();
    // static void DestroyDeviceInfo(webrtc::VideoCaptureModule::DeviceInfo* info);
};

#endif
