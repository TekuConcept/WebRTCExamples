/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree.
 * 
 *  Referenced from modules/audio_device/dummy/file_audio_device.cc
 */

#include "ffmpeg_audio_device.h"

#include <string.h>
#include <sstream>

#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/platform_thread.h"
#include "rtc_base/time_utils.h"
#include "system_wrappers/include/sleep.h"


const int kRecordingFixedSampleRate = 48000;
const size_t kRecordingNumChannels = 2;
const int kPlayoutFixedSampleRate = 48000;
const size_t kPlayoutNumChannels = 2;
const size_t kPlayoutBufferSize =
    kPlayoutFixedSampleRate / 100 * kPlayoutNumChannels * 2;
const size_t kRecordingBufferSize =
    kRecordingFixedSampleRate / 100 * kRecordingNumChannels * 2;

FFmpegAudioDevice::FFmpegAudioDevice()
    : _ptrAudioBuffer(NULL),
      _recordingBuffer(NULL),
      _playoutBuffer(NULL),
      _recordingFramesLeft(0),
      _playoutFramesLeft(0),
      _recordingBufferSizeIn10MS(0),
      _recordingFramesIn10MS(0),
      _playoutFramesIn10MS(0),
      _playing(false),
      _recording(false),
      _lastCallPlayoutMillis(0),
      _lastCallRecordMillis(0),
      // _outputFile(*webrtc::FileWrapper::Create()),
      // _inputFile(*FileWrapper::Create()),
      _inputStream(NULL),
      _outputFilename("webrtcOutputFile.dat"),
      _inputFilename("ffmpegInputStream.pipe")
{ }

FFmpegAudioDevice::~FFmpegAudioDevice() {
  delete &_outputFile;
//   delete &_inputFile;
  if (_inputStream != NULL) {
    fflush(_inputStream);
    pclose(_inputStream);
    _inputStream = NULL;
  }
}

int32_t FFmpegAudioDevice::ActiveAudioLayer(
    webrtc::AudioDeviceModule::AudioLayer& audioLayer) const {
  return -1;
}

webrtc::AudioDeviceGeneric::InitStatus FFmpegAudioDevice::Init() {
  return InitStatus::OK;
}

int32_t FFmpegAudioDevice::Terminate() {
  return 0;
}

bool FFmpegAudioDevice::Initialized() const {
  return true;
}

int16_t FFmpegAudioDevice::PlayoutDevices() {
  return 1;
}

int16_t FFmpegAudioDevice::RecordingDevices() {
  return 1;
}

int32_t FFmpegAudioDevice::PlayoutDeviceName(uint16_t index,
                                           char name[webrtc::kAdmMaxDeviceNameSize],
                                           char guid[webrtc::kAdmMaxGuidSize]) {
  const char* kName = "dummy_device";
  const char* kGuid = "dummy_device_unique_id";
  if (index < 1) {
    memset(name, 0, webrtc::kAdmMaxDeviceNameSize);
    memset(guid, 0, webrtc::kAdmMaxGuidSize);
    memcpy(name, kName, strlen(kName));
    memcpy(guid, kGuid, strlen(guid));
    return 0;
  }
  return -1;
}

int32_t FFmpegAudioDevice::RecordingDeviceName(uint16_t index,
                                             char name[webrtc::kAdmMaxDeviceNameSize],
                                             char guid[webrtc::kAdmMaxGuidSize]) {
  const char* kName = "dummy_device";
  const char* kGuid = "dummy_device_unique_id";
  if (index < 1) {
    memset(name, 0, webrtc::kAdmMaxDeviceNameSize);
    memset(guid, 0, webrtc::kAdmMaxGuidSize);
    memcpy(name, kName, strlen(kName));
    memcpy(guid, kGuid, strlen(guid));
    return 0;
  }
  return -1;
}

int32_t FFmpegAudioDevice::SetPlayoutDevice(uint16_t index) {
  if (index == 0) {
    _playout_index = index;
    return 0;
  }
  return -1;
}

int32_t FFmpegAudioDevice::SetPlayoutDevice(
    webrtc::AudioDeviceModule::WindowsDeviceType device) {
  return -1;
}

int32_t FFmpegAudioDevice::SetRecordingDevice(uint16_t index) {
  if (index == 0) {
    _record_index = index;
    return _record_index;
  }
  return -1;
}

int32_t FFmpegAudioDevice::SetRecordingDevice(
    webrtc::AudioDeviceModule::WindowsDeviceType device) {
  return -1;
}

int32_t FFmpegAudioDevice::PlayoutIsAvailable(bool& available) {
  if (_playout_index == 0) {
    available = true;
    return _playout_index;
  }
  available = false;
  return -1;
}

int32_t FFmpegAudioDevice::InitPlayout() {
  // rtc::CritScope lock(&_critSect);
  webrtc::MutexLock lock(&mutex_);

  if (_playing) {
    return -1;
  }

  _playoutFramesIn10MS = static_cast<size_t>(kPlayoutFixedSampleRate / 100);

  if (_ptrAudioBuffer) {
    // Update webrtc audio buffer with the selected parameters
    _ptrAudioBuffer->SetPlayoutSampleRate(kPlayoutFixedSampleRate);
    _ptrAudioBuffer->SetPlayoutChannels(kPlayoutNumChannels);
  }
  return 0;
}

bool FFmpegAudioDevice::PlayoutIsInitialized() const {
  return _playoutFramesIn10MS != 0;
}

int32_t FFmpegAudioDevice::RecordingIsAvailable(bool& available) {
  if (_record_index == 0) {
    available = true;
    return _record_index;
  }
  available = false;
  return -1;
}

int32_t FFmpegAudioDevice::InitRecording() {
  // rtc::CritScope lock(&_critSect);
  webrtc::MutexLock lock(&mutex_);

  if (_recording) {
    return -1;
  }

  _recordingFramesIn10MS = static_cast<size_t>(kRecordingFixedSampleRate / 100);

  if (_ptrAudioBuffer) {
    _ptrAudioBuffer->SetRecordingSampleRate(kRecordingFixedSampleRate);
    _ptrAudioBuffer->SetRecordingChannels(kRecordingNumChannels);
  }
  return 0;
}

bool FFmpegAudioDevice::RecordingIsInitialized() const {
  return _recordingFramesIn10MS != 0;
}

int32_t FFmpegAudioDevice::StartPlayout() {
  if (_playing) {
    return 0;
  }

  _playing = true;
  _playoutFramesLeft = 0;

  if (!_playoutBuffer) {
    _playoutBuffer = new int8_t[kPlayoutBufferSize];
  }
  if (!_playoutBuffer) {
    _playing = false;
    return -1;
  }

  // PLAYOUT
  // if (!_outputFilename.empty() &&
  //     !_outputFile.OpenFile(_outputFilename.c_str(), false)) {
  if (!_outputFilename.empty()) {
    _outputFile = webrtc::FileWrapper::OpenWriteOnly(_outputFilename.c_str());
    if (!_outputFile.is_open()) {
      RTC_LOG(LS_ERROR) << "Failed to open playout file: " << _outputFilename;
      _playing = false;
      delete[] _playoutBuffer;
      _playoutBuffer = NULL;
      return -1;
    }
  }

  _ptrThreadPlay.reset(new rtc::PlatformThread(
      PlayThreadFunc, this, "webrtc_audio_module_play_thread"));
  _ptrThreadPlay->Start();
  // _ptrThreadPlay->SetPriority(rtc::kRealtimePriority);

  RTC_LOG(LS_INFO) << "Started playout capture to output file: "
                   << _outputFilename;
  return 0;
}

int32_t FFmpegAudioDevice::StopPlayout() {
  {
    // rtc::CritScope lock(&_critSect);
    webrtc::MutexLock lock(&mutex_);
    _playing = false;
  }

  // stop playout thread first
  if (_ptrThreadPlay) {
    _ptrThreadPlay->Stop();
    _ptrThreadPlay.reset();
  }

  // rtc::CritScope lock(&_critSect);
  webrtc::MutexLock lock(&mutex_);

  _playoutFramesLeft = 0;
  delete[] _playoutBuffer;
  _playoutBuffer = NULL;
  // _outputFile.CloseFile();
  _outputFile.Close();

  RTC_LOG(LS_INFO) << "Stopped playout capture to output file: "
                   << _outputFilename;
  return 0;
}

bool FFmpegAudioDevice::Playing() const {
  return _playing;
}

int32_t FFmpegAudioDevice::StartRecording() {
  _recording = true;

  // Make sure we only create the buffer once.
  _recordingBufferSizeIn10MS =
      _recordingFramesIn10MS * kRecordingNumChannels * 2;
  if (!_recordingBuffer) {
    _recordingBuffer = new int8_t[_recordingBufferSizeIn10MS];
  }

  std::ostringstream command;
  command << "/usr/local/bin/ffmpeg";
//   command << " -rtsp_transport tcp";
  command << " -i rtmp://localhost/camera -vn";
  command << " -f s16le -c:a pcm_s16le";
  command << " -ac 2"; // number of channels
  command << " -ar " << (kRecordingFixedSampleRate);
  command << " pipe:";

  if ((_inputStream != NULL) ||
    ((_inputStream = popen(command.str().c_str(), "r")) == NULL)) {
    RTC_LOG(LS_ERROR) << "Failed to open audio input file: " << _inputFilename;
    _recording = false;
    delete[] _recordingBuffer;
    _recordingBuffer = NULL;
    return -1;
  }


  _ptrThreadRec.reset(new rtc::PlatformThread(
      RecThreadFunc, this, "webrtc_audio_module_capture_thread"));

  _ptrThreadRec->Start();
  // _ptrThreadRec->SetPriority(rtc::kRealtimePriority);

  RTC_LOG(LS_INFO) << "Started recording from input file: " << _inputFilename;

  return 0;
}

int32_t FFmpegAudioDevice::StopRecording() {
  {
    // rtc::CritScope lock(&_critSect);
    webrtc::MutexLock lock(&mutex_);
    _recording = false;
  }

  if (_ptrThreadRec) {
    _ptrThreadRec->Stop();
    _ptrThreadRec.reset();
  }

  // rtc::CritScope lock(&_critSect);
  webrtc::MutexLock lock(&mutex_);
  _recordingFramesLeft = 0;
  if (_recordingBuffer) {
    delete[] _recordingBuffer;
    _recordingBuffer = NULL;
  }
//   _inputFile.CloseFile();
  fflush(_inputStream);
  pclose(_inputStream);
  _inputStream = NULL;

  RTC_LOG(LS_INFO) << "Stopped recording from input file: " << _inputFilename;
  return 0;
}

bool FFmpegAudioDevice::Recording() const {
  return _recording;
}

int32_t FFmpegAudioDevice::InitSpeaker() {
  return -1;
}

bool FFmpegAudioDevice::SpeakerIsInitialized() const {
  return false;
}

int32_t FFmpegAudioDevice::InitMicrophone() {
  return 0;
}

bool FFmpegAudioDevice::MicrophoneIsInitialized() const {
  return true;
}

int32_t FFmpegAudioDevice::SpeakerVolumeIsAvailable(bool& available) {
  return -1;
}

int32_t FFmpegAudioDevice::SetSpeakerVolume(uint32_t volume) {
  return -1;
}

int32_t FFmpegAudioDevice::SpeakerVolume(uint32_t& volume) const {
  return -1;
}

int32_t FFmpegAudioDevice::MaxSpeakerVolume(uint32_t& maxVolume) const {
  return -1;
}

int32_t FFmpegAudioDevice::MinSpeakerVolume(uint32_t& minVolume) const {
  return -1;
}

int32_t FFmpegAudioDevice::MicrophoneVolumeIsAvailable(bool& available) {
  return -1;
}

int32_t FFmpegAudioDevice::SetMicrophoneVolume(uint32_t volume) {
  return -1;
}

int32_t FFmpegAudioDevice::MicrophoneVolume(uint32_t& volume) const {
  return -1;
}

int32_t FFmpegAudioDevice::MaxMicrophoneVolume(uint32_t& maxVolume) const {
  return -1;
}

int32_t FFmpegAudioDevice::MinMicrophoneVolume(uint32_t& minVolume) const {
  return -1;
}

int32_t FFmpegAudioDevice::SpeakerMuteIsAvailable(bool& available) {
  return -1;
}

int32_t FFmpegAudioDevice::SetSpeakerMute(bool enable) {
  return -1;
}

int32_t FFmpegAudioDevice::SpeakerMute(bool& enabled) const {
  return -1;
}

int32_t FFmpegAudioDevice::MicrophoneMuteIsAvailable(bool& available) {
  return -1;
}

int32_t FFmpegAudioDevice::SetMicrophoneMute(bool enable) {
  return -1;
}

int32_t FFmpegAudioDevice::MicrophoneMute(bool& enabled) const {
  return -1;
}

int32_t FFmpegAudioDevice::StereoPlayoutIsAvailable(bool& available) {
  available = true;
  return 0;
}

int32_t FFmpegAudioDevice::SetStereoPlayout(bool enable) {
  return 0;
}

int32_t FFmpegAudioDevice::StereoPlayout(bool& enabled) const {
  enabled = true;
  return 0;
}

int32_t FFmpegAudioDevice::StereoRecordingIsAvailable(bool& available) {
  available = true;
  return 0;
}

int32_t FFmpegAudioDevice::SetStereoRecording(bool enable) {
  return 0;
}

int32_t FFmpegAudioDevice::StereoRecording(bool& enabled) const {
  enabled = true;
  return 0;
}

int32_t FFmpegAudioDevice::PlayoutDelay(uint16_t& delayMS) const {
  return 0;
}

void FFmpegAudioDevice::AttachAudioBuffer(webrtc::AudioDeviceBuffer* audioBuffer) {
  // rtc::CritScope lock(&_critSect);
  webrtc::MutexLock lock(&mutex_);

  _ptrAudioBuffer = audioBuffer;

  // Inform the AudioBuffer about default settings for this implementation.
  // Set all values to zero here since the actual settings will be done by
  // InitPlayout and InitRecording later.
  _ptrAudioBuffer->SetRecordingSampleRate(0);
  _ptrAudioBuffer->SetPlayoutSampleRate(0);
  _ptrAudioBuffer->SetRecordingChannels(0);
  _ptrAudioBuffer->SetPlayoutChannels(0);
}

// bool FFmpegAudioDevice::PlayThreadFunc(void* pThis) {
//   return (static_cast<FFmpegAudioDevice*>(pThis)->PlayThreadProcess());
// }
void FFmpegAudioDevice::PlayThreadFunc(void* pThis) {
  FFmpegAudioDevice* device = static_cast<FFmpegAudioDevice*>(pThis);
  while (device->PlayThreadProcess()) { }
}

// bool FFmpegAudioDevice::RecThreadFunc(void* pThis) {
//   return (static_cast<FFmpegAudioDevice*>(pThis)->RecThreadProcess());
// }
void FFmpegAudioDevice::RecThreadFunc(void* pThis) {
  FFmpegAudioDevice* device = static_cast<FFmpegAudioDevice*>(pThis);
  while (device->RecThreadProcess()) { }
}

bool FFmpegAudioDevice::PlayThreadProcess() {
  if (!_playing) {
    return false;
  }
  int64_t currentTime = rtc::TimeMillis();
  // _critSect.Enter();
  mutex_.Lock();

  if (_lastCallPlayoutMillis == 0 ||
      currentTime - _lastCallPlayoutMillis >= 10) {
    // _critSect.Leave();
    mutex_.Unlock();
    _ptrAudioBuffer->RequestPlayoutData(_playoutFramesIn10MS);
    // _critSect.Enter();
    mutex_.Lock();

    _playoutFramesLeft = _ptrAudioBuffer->GetPlayoutData(_playoutBuffer);
    RTC_DCHECK_EQ(_playoutFramesIn10MS, _playoutFramesLeft);
    if (_outputFile.is_open()) {
      _outputFile.Write(_playoutBuffer, kPlayoutBufferSize);
    }
    _lastCallPlayoutMillis = currentTime;
  }
  _playoutFramesLeft = 0;
  // _critSect.Leave();
  mutex_.Unlock();

  int64_t deltaTimeMillis = rtc::TimeMillis() - currentTime;
  if (deltaTimeMillis < 10) {
    webrtc::SleepMs(10 - deltaTimeMillis);
  }

  return true;
}

bool FFmpegAudioDevice::RecThreadProcess() {
  if (!_recording) {
    return false;
  }

  int64_t currentTime = rtc::TimeMillis();
  // _critSect.Enter();
  mutex_.Lock();

  if (_lastCallRecordMillis == 0 || (currentTime - _lastCallRecordMillis) >= 10) {
    // if (_inputFile.is_open()) {
    //   if (_inputFile.Read(_recordingBuffer, kRecordingBufferSize) > 0) {
    //     _ptrAudioBuffer->SetRecordedBuffer(_recordingBuffer,
    //                                        _recordingFramesIn10MS);
    //   } else {
    //     _inputFile.Rewind();
    //   }
    //   _lastCallRecordMillis = currentTime;
    //   _critSect.Leave();
    //   _ptrAudioBuffer->DeliverRecordedData();
    //   _critSect.Enter();
    // }
    if (_inputStream != NULL) {
      size_t bytes_read = fread(_recordingBuffer, 1, kRecordingBufferSize, _inputStream);
      if (static_cast<int>(bytes_read) > 0) {
        _ptrAudioBuffer->SetRecordedBuffer(_recordingBuffer,
                                           _recordingFramesIn10MS);
      }
      _lastCallRecordMillis = currentTime;
      // _critSect.Leave();
      mutex_.Unlock();
      _ptrAudioBuffer->DeliverRecordedData();
      // _critSect.Enter();
      mutex_.Lock();
    }
  }

  // _critSect.Leave();
  mutex_.Unlock();

  int64_t deltaTimeMillis = rtc::TimeMillis() - currentTime;
  if (deltaTimeMillis < 10) {
    webrtc::SleepMs(10 - deltaTimeMillis);
  }

  return true;
}
