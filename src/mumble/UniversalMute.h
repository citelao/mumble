// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MUMBLE_UNIVERSAL_MUTE_H_
#define MUMBLE_MUMBLE_UNIVERSAL_MUTE_H_

#include <functional>
#include <memory>
#include <string>

// UniversalMuter integrates with the Windows VoipCallCoordinator API to support
// the Windows 11 22H2+ Universal Mute feature. All WinRT headers are confined to
// UniversalMute.cpp to avoid _COROUTINE_ABI mismatches with Qt-compiled objects.
class UniversalMuter {
public:
	UniversalMuter();
	UniversalMuter(std::function< void() > onMuted, std::function< void() > onUnmuted);
	~UniversalMuter();

	UniversalMuter(UniversalMuter &&);
	UniversalMuter &operator=(UniversalMuter &&);
	UniversalMuter(const UniversalMuter &)       = delete;
	UniversalMuter &operator=(const UniversalMuter &) = delete;

	void setMuted();
	void setUnmuted();

	void startCall();
	void tryEndCall();

	// callName must be pre-converted from QString by the caller;
	// Qt headers cannot be included in UniversalMute.cpp due to _COROUTINE_ABI constraints.
	void trySetCallName(const std::wstring &callName);

private:
	struct Impl;
	std::unique_ptr< Impl > m_impl;
};

#endif // MUMBLE_MUMBLE_UNIVERSAL_MUTE_H_
