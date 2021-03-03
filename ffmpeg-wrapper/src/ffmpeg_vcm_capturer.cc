/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "ffmpeg_vcm_capturer.h"

#include <stdint.h>

#include <memory>

#include "modules/video_capture/video_capture_factory.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"

#include "examples/peerconnection/client/ffmpeg/ffmpeg_video_factory.h"


FFmpegVcmCapturer::FFmpegVcmCapturer()
: vcm_(nullptr)
{ }


bool
FFmpegVcmCapturer::Init(
    std::string input,
    size_t      width,
    size_t      height,
    size_t      target_fps)
{
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> device_info(
        FFmpegVideoFactory::CreateDeviceInfo());

    char device_name[256];
    char unique_name[256];
    if (device_info->GetDeviceName(
        0u,
        device_name, sizeof(device_name),
        unique_name, sizeof(unique_name)) != 0)
    {
        Destroy();
        return false;
    }

    vcm_ = webrtc::VideoCaptureFactory::Create(unique_name);
    if (!vcm_) return false;
    vcm_->RegisterCaptureDataCallback(this);

    device_info->GetCapability(vcm_->CurrentDeviceName(), 0, capability_);

    capability_.width = static_cast<int32_t>(width);
    capability_.height = static_cast<int32_t>(height);
    capability_.maxFPS = static_cast<int32_t>(target_fps);
    capability_.videoType = webrtc::VideoType::kI420;

    if (vcm_->StartCapture(capability_) != 0) {
        Destroy();
        return false;
    }

    RTC_CHECK(vcm_->CaptureStarted());

    return true;
}


FFmpegVcmCapturer*
FFmpegVcmCapturer::Create(
    std::string input,
    size_t      width,
    size_t      height,
    size_t      target_fps)
{
    std::unique_ptr<FFmpegVcmCapturer> vcm_capturer(new FFmpegVcmCapturer());
    if (!vcm_capturer->Init(input, width, height, target_fps)) {
        RTC_LOG(LS_WARNING) << "Failed to create VcmCapturer(w = " << width
                            << ", h = " << height << ", fps = " << target_fps
                            << ")";
        return nullptr;
    }
    return vcm_capturer.release();
}


void
FFmpegVcmCapturer::Destroy()
{
    if (!vcm_) return;
    vcm_->StopCapture();
    vcm_->DeRegisterCaptureDataCallback();
    vcm_ = nullptr; // Release reference to VCM.
}


FFmpegVcmCapturer::~FFmpegVcmCapturer()
{ Destroy(); }


void
FFmpegVcmCapturer::OnFrame(const webrtc::VideoFrame& frame)
{ webrtc::test::TestVideoCapturer::OnFrame(frame); }
