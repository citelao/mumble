// Copyright 2009-2022 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "UniversalMute.h"
//
//#include "MumbleApplication.h"
//
#include "win.h"
//
//#include <QtCore/QFileInfo>
//#include <QtCore/QSettings>
//#include <QtCore/QString>
//#include <QtCore/QUrl>
//#include <QtCore/QUrlQuery>
//
//// We disable clang-format for these includes as apparently the order in which they are
//// included is important.
//// clang-format off
//#include <shlobj.h>
//#include <shobjidl.h>
//#include <propkey.h>
//#include <propvarutil.h>
//// clang-format on
//
// extern bool bIsWin7;

#include <winrt/windows.applicationmodel.calls.h>

namespace winrt {
using namespace winrt::Windows::ApplicationModel::Calls;
}

void UniversalMute::startCall() {
	const auto todoMuteStateChangedHandler = [this](auto e, auto args) -> {
		if (m_call)
		{
			if (args.Muted) {
				// if (m_onMuted())
				{ coordinator.NotifyMuted(); }
			} else {
				// if (m_onUnmuted())
				{ coordinator.NotifyUnmuted(); }
			}
		}
	};
	// m_coordinator.MuteStateChanged += todoMuteStateChangedHandler
}

void UniversalMuter::startCall() {
	// TODO: args.
	m_call = coordinator.RequestNewOutgoingCall("context_link_todo", "(Connecting...)", "Mumble",
												winrt::VoipPhoneCallMedia::Audio);
}

void UniversalMuter::tryEndCall() {
	if (m_call) {
		m_call.NotifyCallEnded();
		m_call = VoipPhoneCall{ nullptr };
	}
}

void UniversalMuter::trySetCallName(const QString &callName) {
	if (m_call) {
		m_call.ContactName(callName.toStdWString());
	}
}

void UniversalMuter::setMuted() {
	m_coordinator.NotifyMuted();
}

void UniversalMuter::setUnmuted() {
	m_coordinator.NotifyUnmuted();
}
