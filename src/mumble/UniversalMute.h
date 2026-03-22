// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MUMBLE_UNIVERSAL_MUTE_H_
#define MUMBLE_MUMBLE_UNIVERSAL_MUTE_H_

#include <functional>
#include <memory>
#include <string>

// A wrapper around the Windows 10 VoIP Call API, specifically enough to
// interact with the Universal Mute feature. This ensures physical mute buttons
// in recent laptops can mute Mumble.
//
// The feature is only supported on Windows 10 22H2 and later, but this class
// gracefully no-ops on unsupported platforms.
//
// https://stackoverflow.com/questions/74683703/how-do-i-support-call-mute-universal-mute-in-my-app-for-windows-11-22h2
class UniversalMuter {
public:
	UniversalMuter();
	UniversalMuter(std::function< void() > onMuted, std::function< void() > onUnmuted);
	~UniversalMuter();

	UniversalMuter(UniversalMuter &&);
	UniversalMuter &operator=(UniversalMuter &&);
	UniversalMuter(const UniversalMuter &) = delete;
	UniversalMuter &operator=(const UniversalMuter &) = delete;

	void setMuted();
	void setUnmuted();

	void startCall();
	void tryEndCall();

	void trySetCallName(const std::wstring &callName);

private:
	// Use the PIMPL pattern to avoid including WRL/WinRT headers in the public header.
	struct Impl;
	std::unique_ptr< Impl > m_impl;
};

#endif // MUMBLE_MUMBLE_UNIVERSAL_MUTE_H_
