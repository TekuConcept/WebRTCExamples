/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef FFMPEG_VCM_CAPTURER_H_
#define FFMPEG_VCM_CAPTURER_H_

#include <memory>
#include <vector>
#include <string>

#include "api/scoped_refptr.h"
#include "modules/video_capture/video_capture.h"
#include "test/test_video_capturer.h"

class FFmpegVcmCapturer :
    public webrtc::test::TestVideoCapturer,
    public rtc::VideoSinkInterface<webrtc::VideoFrame> {
public:
    static FFmpegVcmCapturer* Create(
        std::string input,
        size_t width,
        size_t height,
        size_t target_fps);
    virtual ~FFmpegVcmCapturer();

    void OnFrame(const webrtc::VideoFrame& frame) override;

private:
    FFmpegVcmCapturer();
    bool Init(
        std::string input,
        size_t width,
        size_t height,
        size_t target_fps);
    void Destroy();

    rtc::scoped_refptr<webrtc::VideoCaptureModule> vcm_;
    webrtc::VideoCaptureCapability capability_;
};

#endif  // TEST_VCM_CAPTURER_H_
