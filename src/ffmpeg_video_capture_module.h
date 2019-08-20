/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree.
 * 
 *  Referenced from modules/video_capture/video_capture_impl.h
 */

#ifndef DEMO_FFMPEG_VIDEO_CAPTURE_MODULE_H_
#define DEMO_FFMPEG_VIDEO_CAPTURE_MODULE_H_

#include <cstdio> // FILE, popen(), pclose(), fread(), fflush()
#include <string>
#include <vector>
#include "rtc_base/criticalsection.h"
#include "rtc_base/platform_thread.h"
#include "modules/video_capture/video_capture.h"


class FFmpegVideoCaptureModule : public webrtc::VideoCaptureModule {
public:
    FFmpegVideoCaptureModule(std::string deviceId);
    ~FFmpegVideoCaptureModule();

    static DeviceInfo* CreateDeviceInfo();

    //   Register capture data callback
    void RegisterCaptureDataCallback(
        rtc::VideoSinkInterface<webrtc::VideoFrame>* dataCallback);

    //  Remove capture data callback
    void DeRegisterCaptureDataCallback();

    // Start capture device
    int32_t StartCapture(const webrtc::VideoCaptureCapability& capability);

    int32_t StopCapture();

    // Returns the name of the device used by this module.
    const char* CurrentDeviceName() const;

    // Returns true if the capture device is running
    bool CaptureStarted();

    // Gets the current configuration.
    int32_t CaptureSettings(webrtc::VideoCaptureCapability& settings);

    // Set the rotation of the captured frames.
    // If the rotation is set to the same as returned by
    // DeviceInfo::GetOrientation the captured frames are
    // displayed correctly if rendered.
    int32_t SetCaptureRotation(webrtc::VideoRotation rotation);

    // Tells the capture module whether to apply the pending rotation. By default,
    // the rotation is applied and the generated frame is up right. When set to
    // false, generated frames will carry the rotation information from
    // SetCaptureRotation. Return value indicates whether this operation succeeds.
    bool SetApplyRotation(bool enable);

    // Return whether the rotation is applied or left pending.
    bool GetApplyRotation();

private:
    struct DeviceMeta {
        std::string name;
        std::string id;
        std::string product;
        webrtc::VideoRotation orientation;
        std::vector<webrtc::VideoCaptureCapability> capabilities;
    };

    rtc::VideoSinkInterface<webrtc::VideoFrame>* dataCallback_;
    std::unique_ptr<rtc::PlatformThread> captureThread_;
    rtc::CriticalSection captureCriticalSection_;

    std::string deviceId_;
    FILE* deviceFd_;
    std::vector<uint8_t> rawFrameBuffer_;

    bool captureStarted_;
    size_t frameCount_;
    webrtc::VideoCaptureCapability currentCapability_;

    // hard-coded ffmpeg devices
    static std::vector<DeviceMeta>* GetDevices();

    // async functions
    static bool CaptureThread(void* object);
    bool CaptureProcess();
    int32_t CheckI420AndPush(
        uint8_t* videoFrame,
        size_t videoFrameLength,
        const webrtc::VideoCaptureCapability& frameInfo,
        int64_t captureTime = 0);

public:
    class FFmpegVideoDeviceInfo : public webrtc::VideoCaptureModule::DeviceInfo {
     public:
        FFmpegVideoDeviceInfo();
        ~FFmpegVideoDeviceInfo();

        uint32_t NumberOfDevices();

        // Returns the available capture devices.
        // deviceNumber   - Index of capture device.
        // deviceNameUTF8 - Friendly name of the capture device.
        // deviceUniqueIdUTF8 - Unique name of the capture device if it exist.
        //                      Otherwise same as deviceNameUTF8.
        // productUniqueIdUTF8 - Unique product id if it exist.
        //                       Null terminated otherwise.
        int32_t GetDeviceName(
            uint32_t deviceNumber,
            char*    deviceNameUTF8,
            uint32_t deviceNameLength,
            char*    deviceUniqueIdUTF8,
            uint32_t deviceUniqueIdUTF8Length,
            char*    productUniqueIdUTF8 = 0,
            uint32_t productUniqueIdUTF8Length = 0);

        // Returns the number of capabilities this device.
        int32_t NumberOfCapabilities(const char* deviceUniqueIdUTF8);

        // Gets the capabilities of the named device.
        int32_t GetCapability(
            const char* deviceUniqueIdUTF8,
            const uint32_t deviceCapabilityNumber,
            webrtc::VideoCaptureCapability& capability);

        // Gets clockwise angle the captured frames should be rotated in order
        // to be displayed correctly on a normally rotated display.
        int32_t GetOrientation(
            const char* deviceUniqueIdUTF8,
            webrtc::VideoRotation& orientation);

        // Gets the capability that best matches the requested width, height and
        // frame rate.
        // Returns the deviceCapabilityNumber on success.
        int32_t GetBestMatchedCapability(
            const char* deviceUniqueIdUTF8,
            const webrtc::VideoCaptureCapability& requested,
            webrtc::VideoCaptureCapability& resulting);

        // Display OS /capture device specific settings dialog
        int32_t DisplayCaptureSettingsDialogBox(
            const char* deviceUniqueIdUTF8,
            const char* dialogTitleUTF8,
            void* parentWindow,
            uint32_t positionX,
            uint32_t positionY);

     private:
        std::vector<DeviceMeta>& devices_;

        inline int32_t NextMatchScore(
            const webrtc::VideoCaptureCapability& target,
            const webrtc::VideoCaptureCapability& next);
    };
};

#endif
