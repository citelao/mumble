// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

// NOTE: Do NOT include any Qt headers in this file.
// The WinRT headers below force _COROUTINE_ABI=2 via a linker /FAILIFMISMATCH pragma.
// Qt libs in the vcpkg environment are compiled with _COROUTINE_ABI=1.
// Mixing Qt includes here would propagate the ABI conflict into a translation unit
// that also needs Qt symbols. Keep this file WinRT/stdlib only.
// See: https://github.com/microsoft/cppwinrt/issues/1281

#include "UniversalMute.h"

#include "win.h"

#include <winrt/Windows.ApplicationModel.Calls.h>

namespace winrt {
using namespace winrt::Windows::ApplicationModel::Calls;
}

struct UniversalMuter::Impl {
	std::function< void() > onMuted;
	std::function< void() > onUnmuted;
	winrt::event_token muteStateToken{};
	winrt::VoipCallCoordinator coordinator{ nullptr };
	winrt::VoipPhoneCall call{ nullptr };
};

UniversalMuter::UniversalMuter(std::function< void() > onMuted, std::function< void() > onUnmuted)
	: m_impl(std::make_unique< Impl >()) {
	m_impl->onMuted   = std::move(onMuted);
	m_impl->onUnmuted = std::move(onUnmuted);
	m_impl->coordinator = winrt::VoipCallCoordinator::GetDefault();
	m_impl->muteStateToken =
		m_impl->coordinator.MuteStateChanged([this](const winrt::VoipCallCoordinator &,
												 const winrt::MuteChangeEventArgs &args) {
			if (args.Muted()) {
				if (m_impl->onMuted)
					m_impl->onMuted();
				m_impl->coordinator.NotifyMuted();
			} else {
				if (m_impl->onUnmuted)
					m_impl->onUnmuted();
				m_impl->coordinator.NotifyUnmuted();
			}
		});
}

UniversalMuter::~UniversalMuter() {
	if (m_impl && m_impl->coordinator) {
		m_impl->coordinator.MuteStateChanged(m_impl->muteStateToken);
	}
	tryEndCall();
}

void UniversalMuter::startCall() {
	if (!m_impl || !m_impl->coordinator)
		return;
	try {
		m_impl->call = m_impl->coordinator.RequestNewOutgoingCall(
			L"", L"Connecting...", L"Mumble", winrt::VoipPhoneCallMedia::Audio);
	} catch (const winrt::hresult_error &) {
		// App may lack package identity; mute state notifications still work without a call object.
	}
}

void UniversalMuter::tryEndCall() {
	if (!m_impl || !m_impl->call)
		return;
	m_impl->call.NotifyCallEnded();
	m_impl->call = nullptr;
}

void UniversalMuter::trySetCallName(const std::wstring &callName) {
	if (m_impl && m_impl->call)
		m_impl->call.ContactName(callName);
}

void UniversalMuter::setMuted() {
	if (m_impl && m_impl->coordinator)
		m_impl->coordinator.NotifyMuted();
}

void UniversalMuter::setUnmuted() {
	if (m_impl && m_impl->coordinator)
		m_impl->coordinator.NotifyUnmuted();
}
