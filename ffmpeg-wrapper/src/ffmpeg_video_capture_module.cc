/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree.
 * 
 *  Referenced from modules/video_capture/video_capture_impl.cc
 */

#include "ffmpeg_video_capture_module.h"

#include <unistd.h> // usleep()
#include <vector>
#include <sstream>

#include "api/video/i420_buffer.h"
#include "rtc_base/logging.h"
#include "rtc_base/time_utils.h"
#include "system_wrappers/include/clock.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "third_party/libyuv/include/libyuv.h"


FFmpegVideoCaptureModule::FFmpegVideoCaptureModule(std::string deviceId)
{
    deviceId_         = deviceId;
    deviceFd_         = NULL;
    captureStarted_   = false;

    currentCapability_.width     = 0;
    currentCapability_.height    = 0;
    currentCapability_.maxFPS    = 0;
    currentCapability_.videoType = webrtc::VideoType::kUnknown;
}

FFmpegVideoCaptureModule::~FFmpegVideoCaptureModule()
{
    StopCapture();
    if (deviceFd_ != NULL) {
        fflush(deviceFd_);
        pclose(deviceFd_);
        deviceFd_ = NULL;
    }
}


void
FFmpegVideoCaptureModule::RegisterCaptureDataCallback(
    rtc::VideoSinkInterface<webrtc::VideoFrame>* dataCallback)
{
    // rtc::CritScope cs(&captureCriticalSection_);
    webrtc::MutexLock lock(&mutex_);
    dataCallback_ = dataCallback;
}


void
FFmpegVideoCaptureModule::DeRegisterCaptureDataCallback()
{
    // rtc::CritScope cs(&captureCriticalSection_);
    webrtc::MutexLock lock(&mutex_);
    dataCallback_ = nullptr;
}


webrtc::VideoCaptureModule::DeviceInfo*
FFmpegVideoCaptureModule::CreateDeviceInfo()
{ return new FFmpegVideoDeviceInfo(); }


int32_t
FFmpegVideoCaptureModule::StartCapture(
    const webrtc::VideoCaptureCapability& capability)
{
    // referenced from video_capture_linux.cc
    if (captureStarted_) {
        if (capability.width     == currentCapability_.width  &&
            capability.height    == currentCapability_.height &&
            capability.videoType == currentCapability_.videoType)
            return 0;
        else StopCapture();
    }

    // rtc::CritScope cs(&captureCriticalSection_);
    webrtc::MutexLock lock(&mutex_);

    // 1. open [named?] pipe
    frameCount_        = 0;
    currentCapability_ = capability;
    std::string pixelFormat;

    {
        auto area = currentCapability_.width * currentCapability_.height;
        switch (capability.videoType) {
        case webrtc::VideoType::kI420:
            rawFrameBuffer_.resize(area * 3 / 2);
            pixelFormat = "yuv420p";
            break;
        case webrtc::VideoType::kRGB24:
            rawFrameBuffer_.resize(area * 3);
            pixelFormat = "rgb24";
            break;
        default:
            RTC_LOG(LS_ERROR) << "Can't predict frame information.";
            return -1;
        }
    }

    std::ostringstream command;
    command << "/usr/local/bin/ffmpeg";
    // command << " -rtsp_transport tcp";
    command << " -i video.h264";
    command << " -f image2pipe -c:v rawvideo -pix_fmt " << pixelFormat;
    command << " -r " << capability.maxFPS; // frames will be dropped if in-fps exceeds out-fps
    command << " -s " << capability.width << "x" << capability.height; // output size
    command << " -"; // pipe
    deviceFd_ = popen(command.str().c_str(), "r");

    // 2. start capture thread;
    if (!captureThread_) {
        captureThread_.reset(new rtc::PlatformThread(
            FFmpegVideoCaptureModule::CaptureThread, this, "CaptureThread"));
        captureThread_->Start();
        // captureThread_->SetPriority(rtc::kHighPriority);
    }

    captureStarted_ = true;
    return 0;
}


int32_t
FFmpegVideoCaptureModule::StopCapture()
{
    if (captureThread_) {
        captureThread_->Stop();
        captureThread_.reset();
    }

    // rtc::CritScope cs(&captureCriticalSection_);
    webrtc::MutexLock lock(&mutex_);
    if (captureStarted_) {
        captureStarted_ = false;
        fflush(deviceFd_);
        pclose(deviceFd_);
        deviceFd_ = NULL;
    }

    return 0;
}


const char*
FFmpegVideoCaptureModule::CurrentDeviceName() const
{ return deviceId_.c_str(); }


bool
FFmpegVideoCaptureModule::CaptureStarted()
{ return captureStarted_; }


int32_t
FFmpegVideoCaptureModule::CaptureSettings(webrtc::VideoCaptureCapability& settings)
{
    settings = currentCapability_;
    return 0;
}


int32_t
FFmpegVideoCaptureModule::SetCaptureRotation(webrtc::VideoRotation /* rotation */)
{ return -1; }


bool
FFmpegVideoCaptureModule::SetApplyRotation(bool /* enable */)
{ return true; }


bool
FFmpegVideoCaptureModule::GetApplyRotation()
{ return false; }


std::vector<FFmpegVideoCaptureModule::DeviceMeta>*
FFmpegVideoCaptureModule::GetDevices()
{
    static bool loaded = false;
    static std::vector<DeviceMeta> devices;
    if (!loaded) {
        DeviceMeta device0;
        device0.name        = std::string("ffmpeg-0");     // arbitrary (name)
        device0.id          = std::string("F3E977DB27F1"); // random (id)
        device0.product     = std::string("A0A0860E9BDC"); // random (product id)
        device0.orientation = webrtc::VideoRotation::kVideoRotation_0;
        {
            webrtc::VideoCaptureCapability capability;
            capability.width      = 640;
            capability.height     = 480;
            capability.maxFPS     = 30;
            capability.interlaced = false;
            capability.videoType  = webrtc::VideoType::kI420;
            device0.capabilities.push_back(capability);
            capability.videoType  = webrtc::VideoType::kRGB24;
            device0.capabilities.push_back(capability);
            // kNV12 ("nv12")
            // kNV21 ("nv21")
            // kARGB ("argb")
        }
        devices.push_back(device0);
        loaded = true;
    }
    return &devices;
}


// bool
// FFmpegVideoCaptureModule::CaptureThread(void* object)
// { return static_cast<FFmpegVideoCaptureModule*>(object)->CaptureProcess(); }
void
FFmpegVideoCaptureModule::CaptureThread(void* object)
// taken from video_capture_linux.cc
{
    FFmpegVideoCaptureModule* module = static_cast<FFmpegVideoCaptureModule*>(object);
    while (module->CaptureProcess()) { }
}


bool
FFmpegVideoCaptureModule::CaptureProcess()
{
    // rtc::CritScope cs(&captureCriticalSection_);
    webrtc::MutexLock lock(&mutex_);

    // do one-cycle's worth of work
    if (captureStarted_) {
        // Read a frame from the input pipe into the buffer
        size_t count = fread(&rawFrameBuffer_[0], 1,
            rawFrameBuffer_.size(), deviceFd_);

        // If we didn't get a frame, we're probably at the end
        if (count != rawFrameBuffer_.size()) return false;

        CheckI420AndPush((unsigned char*)&rawFrameBuffer_[0],
            rawFrameBuffer_.size(), currentCapability_);
    }
    // else do nothing - thread may close soon

    usleep(0);
    return true;
}


int32_t FFmpegVideoCaptureModule::CheckI420AndPush(
    uint8_t* videoFrame,
    size_t videoFrameLength,
    const webrtc::VideoCaptureCapability& frameInfo,
    int64_t captureTime)
{
    const int32_t width = frameInfo.width;
    const int32_t height = frameInfo.height;

    if (frameInfo.videoType != webrtc::VideoType::kMJPEG &&
        webrtc::CalcBufferSize(frameInfo.videoType, width, abs(height)) !=
            videoFrameLength) {
        RTC_LOG(LS_ERROR) << "Wrong incoming frame length.";
        return -1;
    }

    int stride_y      = width;
    int stride_uv     = (width + 1) / 2;
    int target_width  = width;
    int target_height = abs(height);

    rtc::scoped_refptr<webrtc::I420Buffer> buffer = webrtc::I420Buffer::Create(
        target_width, target_height, stride_y, stride_uv, stride_uv);

    const int conversionResult = libyuv::ConvertToI420(
        videoFrame, videoFrameLength,
        buffer.get()->MutableDataY(), buffer.get()->StrideY(),
        buffer.get()->MutableDataU(), buffer.get()->StrideU(),
        buffer.get()->MutableDataV(), buffer.get()->StrideV(),
        0, 0,  // No Cropping
        width, height,
        target_width, target_height,
        libyuv::kRotate0,
        ConvertVideoType(frameInfo.videoType));

    if (conversionResult < 0) {
        RTC_LOG(LS_ERROR) << "Failed to convert capture frame from type "
            << static_cast<int>(frameInfo.videoType) << "to I420.";
        return -1;
    }

    webrtc::VideoFrame captureFrame(buffer, 0, rtc::TimeMillis(),
        webrtc::VideoRotation::kVideoRotation_0);
    captureFrame.set_ntp_time_ms(captureTime);

    frameCount_++;
    if (dataCallback_)
        dataCallback_->OnFrame(captureFrame);

    return 0;
}
