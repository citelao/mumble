// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MUMBLE_COREAUDIO_H_
#	define MUMBLE_MUMBLE_COREAUDIO_H_

#	include "AudioInput.h"
#	include "AudioOutput.h"
#	include <AudioToolbox/AudioToolbox.h>

enum AUDirection { OUTPUT = 0, INPUT = 1 };

class CoreAudioSystem : public QObject {
private:
	Q_OBJECT
	Q_DISABLE_COPY(CoreAudioSystem)
public:
	static const QHash< QString, QString > getDevices(bool input, bool echo);
	static const QList< audioDevice > getDeviceChoices(bool input);
};

class CoreAudioInput : public AudioInput {
private:
	Q_OBJECT
	Q_DISABLE_COPY(CoreAudioInput)
	/// Open HAL AU as input and pass back the output stream description.
	bool openAUHAL(AudioStreamBasicDescription &streamDescription);

	/// Open VoiceProcessingIO AU as input, utilizing macOS's builtin echo cancellation,
	/// and pass back the output stream description.
	bool openAUVoip(AudioStreamBasicDescription &streamDescription);

	/// Initialize input AU with preferred parameters of Mumble
	bool initializeInputAU(AudioUnit au, AudioStreamBasicDescription &streamDescription, int &actualBufferLength);

protected:
	/// Hardware Abstraction Layer's AudioUnit, directly interacts with the hardware
	AudioUnit auHAL{};
	/// VoiceProcessingIO AU, provides audio input and echo cancellation
	AudioUnit auVoip{};
	AudioDeviceID inputDevId{};
	AudioDeviceID echoOutputDevId{};
	AudioBufferList buflist{};
	static void propertyChange(void *udata, AudioUnit au, AudioUnitPropertyID prop, AudioUnitScope scope,
							   AudioUnitElement element);
	static OSStatus deviceChange(AudioObjectID inObjectID, UInt32 inNumberAddresses,
								 const AudioObjectPropertyAddress inAddresses[], void *udata);
	static OSStatus inputCallback(void *udata, AudioUnitRenderActionFlags *flags, const AudioTimeStamp *ts,
								  UInt32 busnum, UInt32 npackets, AudioBufferList *buflist);

#	ifdef USE_WEBRTC_APM
	// System-audio capture via a Core Audio process tap (macOS 14.2+), used as the WebRTC AEC
	// echo reference so echo from ALL applications' output is cancelled — not just Mumble's.
	// A private, mono, global tap is wrapped in a private aggregate device whose IOProc feeds the
	// captured audio to AudioInput::addEchoReference().
	AudioObjectID m_echoTapID{};
	AudioDeviceID m_echoAggregateID{};
	AudioDeviceIOProcID m_echoIOProcID{};
	SpeexResamplerState *m_echoTapResampler{};
	unsigned int m_echoTapFreq{};
	bool startSystemAudioTap();
	void stopSystemAudioTap();
	static OSStatus echoTapIOProc(AudioObjectID inDevice, const AudioTimeStamp *inNow,
								  const AudioBufferList *inInputData, const AudioTimeStamp *inInputTime,
								  AudioBufferList *outOutputData, const AudioTimeStamp *inOutputTime,
								  void *inClientData);
#	endif

public:
#	ifdef USE_WEBRTC_APM
	/// Set while the process tap is delivering audio, so CoreAudioOutput knows not to also feed
	/// its own render (which the tap already includes) as a fallback reference.
	static std::atomic< bool > sm_systemTapActive;
#	endif
	CoreAudioInput();
	~CoreAudioInput() Q_DECL_OVERRIDE;
	void run() Q_DECL_OVERRIDE;
	void stop();
};

class CoreAudioOutput : public AudioOutput {
private:
	Q_OBJECT
	Q_DISABLE_COPY(CoreAudioOutput)
protected:
	/// Hardware Abstraction Layer's AudioOutputUnit, directly interacts with the hardware
	AudioUnit auHAL{};
	static void propertyChange(void *udata, AudioUnit au, AudioUnitPropertyID prop, AudioUnitScope scope,
							   AudioUnitElement element);
	static OSStatus deviceChange(AudioObjectID inObjectID, UInt32 inNumberAddresses,
								 const AudioObjectPropertyAddress inAddresses[], void *udata);
	static OSStatus outputCallback(void *udata, AudioUnitRenderActionFlags *flags, const AudioTimeStamp *ts,
								   UInt32 busnum, UInt32 npackets, AudioBufferList *buflist);

#	ifdef USE_WEBRTC_APM
	/// Resampler used to convert the rendered mix to Mumble's sample rate for the WebRTC AEC
	/// echo reference (only allocated when the output device runs at a different rate).
	SpeexResamplerState *srsEcho{};
	/// Downmix the just-rendered mix to a canonical mono stream at Mumble's sample rate and feed
	/// it to the active AudioInput as the WebRTC AEC echo reference. Called from outputCallback.
	void feedWebrtcEchoReference(const void *mixData, unsigned int nframes);
#	endif

public:
	CoreAudioOutput();
	~CoreAudioOutput() Q_DECL_OVERRIDE;
	void run() Q_DECL_OVERRIDE;
	void stop();
};

class CoreAudioInputRegistrar : public AudioInputRegistrar {
public:
	CoreAudioInputRegistrar();
	virtual AudioInput *create();
	virtual const QVariant getDeviceChoice();
	virtual const QList< audioDevice > getDeviceChoices();
	virtual void setDeviceChoice(const QVariant &, Settings &);
	virtual bool canEcho(EchoCancelOptionID echoCancelID, const QString &outputSystem) const;
	virtual bool isMicrophoneAccessDeniedByOS();
};

class CoreAudioOutputRegistrar : public AudioOutputRegistrar {
public:
	CoreAudioOutputRegistrar() : AudioOutputRegistrar(QLatin1String("CoreAudio"), 10) {}
	virtual AudioOutput *create();
	virtual const QVariant getDeviceChoice();
	virtual const QList< audioDevice > getDeviceChoices();
	virtual void setDeviceChoice(const QVariant &, Settings &);
	bool canMuteOthers() const;
};

#else
class CoreAudioSystem;
class CoreAudioInput;
class CoreAudioOutput;
#endif
