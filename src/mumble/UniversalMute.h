// Copyright 2009-2022 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MUMBLE_UNIVRESAL_MUTE_H_
#define MUMBLE_MUMBLE_UNIVRESAL_MUTE_H_

class QString;

#include <winrt/windows.applicationmodel.calls.h>

class UniversalMuter {
public:
	typedef int todobesto;
	UniveralMuter(todobesto onMuted, todobesto onUnmuted)
		: m_onMuted{ onMuted }, m_onUnmuted{ onUnmuted }, m_coordinator{
			  winrt::Windows::ApplicationModel::Calls::VoipCallCoordinator::GetDefault()
		  } {}

	void setMuted();
	void setUnmuted();

	// We could handle an arbitrary number of calls, but only store one to
	// keep the interfaces simple.
	void startCall();
	void tryEndCall();

	void trySetCallName(const QString &callName);

private:
	todobesto m_onMuted;
	todobesto m_onUnmuted;

	winrt::Windows::ApplicationModel::Calls::VoipCallCoordinator m_coordinator{};
	winrt::Windows::ApplicationModel::Calls::VoipPhoneCall m_call{};
};

#endif // MUMBLE_MUMBLE_UNIVRESAL_MUTE_H_
