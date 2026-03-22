// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

// We use WRL (Windows Runtime C++ Template Library) instead of C++/WinRT projection
// headers. cppwinrt adds a hardcoded #pragma comment(linker, "/FAILIFMISMATCH:_COROUTINE_ABI=2")
// which conflicts with Qt libs compiled with _COROUTINE_ABI=1. WRL uses plain COM and has
// no such pragma. See: https://github.com/microsoft/cppwinrt/issues/1281

#include "UniversalMute.h"

#include "win.h"

// clang-format off
#include <wrl.h>
#include <wrl/event.h>
#include <wrl/wrappers/corewrappers.h>
#include <roapi.h>
#include <windows.applicationmodel.calls.h>
// clang-format on

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::ApplicationModel::Calls;
using namespace ABI::Windows::Foundation;

struct UniversalMuter::Impl {
	std::function< void() > onMuted;
	std::function< void() > onUnmuted;
	ComPtr< IVoipCallCoordinator > coordinator;
	ComPtr< IVoipPhoneCall > call;
	EventRegistrationToken muteStateToken{};
};

namespace {
ComPtr< IVoipCallCoordinator > tryCreateCallCoordinator() {
	ComPtr< IVoipCallCoordinatorStatics > statics;
	HRESULT hr = RoGetActivationFactory(
		HStringReference(RuntimeClass_Windows_ApplicationModel_Calls_VoipCallCoordinator).Get(),
		IID_PPV_ARGS(&statics));
	if (FAILED(hr))
		return nullptr;

	ComPtr< IVoipCallCoordinator > coordinator;
	hr = statics->GetDefault(&coordinator);
	if (FAILED(hr))
		return nullptr;

	return coordinator;
}
}

UniversalMuter::UniversalMuter(std::function< void() > onMuted, std::function< void() > onUnmuted)
	: m_impl(std::make_unique< Impl >()) {
	m_impl->onMuted   = std::move(onMuted);
	m_impl->onUnmuted = std::move(onUnmuted);

	// TOOD: initialize pattern
	m_impl->coordinator = tryCreateCallCoordinator();
	if (!m_impl->coordinator)
		return;

	// Capture impl directly rather than 'this': the UniversalMuter may be move-assigned after
	// construction, invalidating 'this', but the Impl object stays at the same heap address.
	Impl *impl = m_impl.get();
	auto handler = Callback< ITypedEventHandler< VoipCallCoordinator *, MuteChangeEventArgs * > >(
		[impl](IVoipCallCoordinator *, IMuteChangeEventArgs *args) -> HRESULT {
			boolean muted = FALSE;
			args->get_Muted(&muted);
			if (muted) {
				if (impl->onMuted)
					impl->onMuted();
				impl->coordinator->NotifyMuted();
			} else {
				if (impl->onUnmuted)
					impl->onUnmuted();
				impl->coordinator->NotifyUnmuted();
			}
			return S_OK;
		});

	m_impl->coordinator->add_MuteStateChanged(handler.Get(), &m_impl->muteStateToken);
}

UniversalMuter::UniversalMuter()                             = default;
UniversalMuter::UniversalMuter(UniversalMuter &&)            = default;
UniversalMuter &UniversalMuter::operator=(UniversalMuter &&) = default;

UniversalMuter::~UniversalMuter() {
	if (m_impl && m_impl->coordinator)
		m_impl->coordinator->remove_MuteStateChanged(m_impl->muteStateToken);
	tryEndCall();
}

void UniversalMuter::startCall() {
	if (!m_impl || !m_impl->coordinator)
		return;

	ComPtr< IVoipPhoneCall > call;
	HString context, contactName, serviceName;
	context.Set(L"");
	contactName.Set(L"Connecting...");
	serviceName.Set(L"Mumble");

	// RequestNewOutgoingCall may fail with E_ACCESSDENIED if the app lacks package identity.
	// Mute state notifications still work without an active call object.
	HRESULT hr = m_impl->coordinator->RequestNewOutgoingCall(context.Get(), contactName.Get(),
															 serviceName.Get(),
															 VoipPhoneCallMedia_Audio, &call);
	if (SUCCEEDED(hr))
		m_impl->call = call;
}

void UniversalMuter::tryEndCall() {
	if (!m_impl || !m_impl->call)
		return;
	m_impl->call->NotifyCallEnded();
	m_impl->call.Reset();
}

void UniversalMuter::trySetCallName(const std::wstring &callName) {
	if (!m_impl || !m_impl->call)
		return;
	HString name;
	name.Set(callName.c_str());
	m_impl->call->put_ContactName(name.Get());
}

void UniversalMuter::setMuted() {
	if (m_impl && m_impl->coordinator)
		m_impl->coordinator->NotifyMuted();
}

void UniversalMuter::setUnmuted() {
	if (m_impl && m_impl->coordinator)
		m_impl->coordinator->NotifyUnmuted();
}
