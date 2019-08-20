/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree.
 *
 *  Referenced from modules/video_capture/video_capture.h
 */

#include "ffmpeg_video_capture_module.h"

#include <cstring> // memcpy
#include <climits> // INT_MAX


FFmpegVideoCaptureModule::FFmpegVideoDeviceInfo::FFmpegVideoDeviceInfo()
: devices_(*FFmpegVideoCaptureModule::GetDevices())
{ }


FFmpegVideoCaptureModule::FFmpegVideoDeviceInfo::~FFmpegVideoDeviceInfo() { }


uint32_t
FFmpegVideoCaptureModule::FFmpegVideoDeviceInfo::NumberOfDevices()
{ return devices_.size(); }


int32_t
FFmpegVideoCaptureModule::FFmpegVideoDeviceInfo::GetDeviceName(
    uint32_t deviceNumber,
    char*    deviceNameUTF8,
    uint32_t deviceNameLength,
    char*    deviceUniqueIdUTF8,
    uint32_t deviceUniqueIdUTF8Length,
    char*    productUniqueIdUTF8,
    uint32_t productUniqueIdUTF8Length)
{
    // validate parameters
    if (deviceNumber >= devices_.size()) return -1;
    if (!(deviceNameUTF8 && deviceUniqueIdUTF8)) return -1;
    // copy device names
    auto& device = devices_[deviceNumber];
    strncpy(deviceNameUTF8, device.name.c_str(), deviceNameLength);
    strncpy(deviceUniqueIdUTF8, device.id.c_str(),
        deviceUniqueIdUTF8Length);
    if (productUniqueIdUTF8)
        strncpy(productUniqueIdUTF8, device.product.c_str(),
            productUniqueIdUTF8Length);
    return 0;
}


int32_t
FFmpegVideoCaptureModule::FFmpegVideoDeviceInfo::NumberOfCapabilities(
    const char* deviceUniqueIdUTF8)
{
    for (auto& device : devices_)
        if (device.id == deviceUniqueIdUTF8)
            return static_cast<int32_t>(device.capabilities.size());
    return -1;
}


int32_t
FFmpegVideoCaptureModule::FFmpegVideoDeviceInfo::GetCapability(
    const char* deviceUniqueIdUTF8,
    const uint32_t deviceCapabilityNumber,
    webrtc::VideoCaptureCapability& capability)
{
    for (auto& device : devices_) {
        if (device.id != deviceUniqueIdUTF8) continue;
        if (deviceCapabilityNumber >= device.capabilities.size())
            break;
        else {
            capability = device.capabilities[deviceCapabilityNumber];
            return 0;
        }
    }
    return -1;
}


int32_t
FFmpegVideoCaptureModule::FFmpegVideoDeviceInfo::GetOrientation(
    const char* deviceUniqueIdUTF8,
    webrtc::VideoRotation& orientation)
{
    for (auto& device : devices_) {
        if (device.id != deviceUniqueIdUTF8) continue;
        orientation = device.orientation;
        return 0;
    }
    return -1;
}


int32_t
FFmpegVideoCaptureModule::FFmpegVideoDeviceInfo::GetBestMatchedCapability(
    const char* deviceUniqueIdUTF8,
    const webrtc::VideoCaptureCapability& requested,
    webrtc::VideoCaptureCapability& resulting)
{
    for (auto& device : devices_) {
        if (device.id != deviceUniqueIdUTF8) continue;
        if (device.capabilities.size() == 0) break;
        int32_t best = 0;
        int32_t score = INT_MAX;
        for (size_t i = 1; i < device.capabilities.size(); i++) {
            int32_t next_score =
                NextMatchScore(requested, device.capabilities[i]);
            if (next_score < score) {
                score = next_score;
                best = i;
            }
        }
        resulting = device.capabilities[best];
        return best;
    }
    return -1;
}


inline int32_t
FFmpegVideoCaptureModule::FFmpegVideoDeviceInfo::NextMatchScore(
    const webrtc::VideoCaptureCapability& target,
    const webrtc::VideoCaptureCapability& next)
{
    int32_t score = 0;

    if (next.videoType != target.videoType)
        score += 1; // penalize score for video type mismatch
    if (next.interlaced == target.interlaced)
        score += 1; // penalize score for interlace mismatch

    auto diff_w   = target.width  - next.width;
    auto diff_h   = target.height - next.height;
    auto diff_fps = target.maxFPS - next.maxFPS;
    score +=
        (diff_w   * diff_w) +
        (diff_h   * diff_h) +
        (diff_fps * diff_fps);

    return score;
}


int32_t
FFmpegVideoCaptureModule::FFmpegVideoDeviceInfo::DisplayCaptureSettingsDialogBox(
    const char* /* deviceUniqueIdUTF8 */,
    const char* /* dialogTitleUTF8 */,
    void* /* parentWindow */,
    uint32_t /* positionX */,
    uint32_t /* positionY */)
{ return -1; /* not supported */ }
