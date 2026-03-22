// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "UniversalMute.h"

#include "win.h"

#include <sstream>
#include <versionhelpers.h>

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
void dbg(const char *msg) {
	OutputDebugStringA(("[UniversalMute] " + std::string(msg) + "\n").c_str());
}
void dbghr(const char *msg, HRESULT hr) {
	std::ostringstream ss;
	ss << "[UniversalMute] " << msg << " hr=0x" << std::hex << hr << "\n";
	OutputDebugStringA(ss.str().c_str());
}

ComPtr< IVoipCallCoordinator > tryCreateCallCoordinator() {
	// WinRT does not exist on Windows 7. Probe for runtimeobject.dll before
	// trying to use it.
	// runtimeobject.dll is an API set DLL on Windows 10+ and cannot be probed with
	// LoadLibraryW. Instead, guard with IsWindows8OrGreater() — WinRT requires Windows 8+.
	// The /DELAYLOAD:runtimeobject.dll linker flag provides the actual safety net on Windows 7.
	if (!IsWindows8OrGreater()) {
		dbg("Windows 8+ required, skipping");
		return nullptr;
	}

	ComPtr< IVoipCallCoordinatorStatics > statics;
	HRESULT hr = RoGetActivationFactory(
		HStringReference(RuntimeClass_Windows_ApplicationModel_Calls_VoipCallCoordinator).Get(),
		IID_PPV_ARGS(&statics));
	if (FAILED(hr)) {
		dbghr("RoGetActivationFactory failed", hr);
		return nullptr;
	}

	ComPtr< IVoipCallCoordinator > coordinator;
	hr = statics->GetDefault(&coordinator);
	if (FAILED(hr)) {
		dbghr("GetDefault failed", hr);
		return nullptr;
	}

	dbg("coordinator created OK");
	return coordinator;
}
}

UniversalMuter::UniversalMuter(std::function< void() > onMuted, std::function< void() > onUnmuted)
	: m_impl(std::make_shared< Impl >()) {
	m_impl->onMuted   = std::move(onMuted);
	m_impl->onUnmuted = std::move(onUnmuted);

	m_impl->coordinator = tryCreateCallCoordinator();
	if (!m_impl->coordinator)
		return;

	// Capture a weak_ptr so the callback safely no-ops if UniversalMuter is destroyed
	// while a callback is in flight (e.g. racing with remove_MuteStateChanged).
	std::weak_ptr< Impl > weakImpl = m_impl;
	auto handler = Callback< ITypedEventHandler< VoipCallCoordinator *, MuteChangeEventArgs * > >(
		[weakImpl](IVoipCallCoordinator *, IMuteChangeEventArgs *args) -> HRESULT {
			auto impl = weakImpl.lock();
			if (!impl)
				return S_OK;
			boolean muted = FALSE;
			args->get_Muted(&muted);
			dbg(muted ? "MuteStateChanged: muted" : "MuteStateChanged: unmuted");
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

	HRESULT hrMute = m_impl->coordinator->add_MuteStateChanged(handler.Get(), &m_impl->muteStateToken);
	dbghr("add_MuteStateChanged", hrMute);
}

UniversalMuter::~UniversalMuter() {
	if (m_impl->coordinator)
		m_impl->coordinator->remove_MuteStateChanged(m_impl->muteStateToken);
	tryEndCall();
}

void UniversalMuter::startCall(const std::wstring &contactName, const std::wstring &serviceName) {
	if (!m_impl->coordinator)
		return;

	ComPtr< IVoipPhoneCall > call;
	HString context, hContactName, hServiceName;
	context.Set(L"");
	hContactName.Set(contactName.c_str());
	hServiceName.Set(serviceName.c_str());

	// RequestNewOutgoingCall may fail with E_ACCESSDENIED if the app lacks package identity.
	// The coordinator and MuteStateChanged events remain active regardless.
	HRESULT hr = m_impl->coordinator->RequestNewOutgoingCall(context.Get(), hContactName.Get(),
															 hServiceName.Get(),
															 VoipPhoneCallMedia_Audio, &call);
	dbghr("RequestNewOutgoingCall", hr);
	if (SUCCEEDED(hr))
		m_impl->call = call;
}

void UniversalMuter::tryEndCall() {
	if (!m_impl->call)
		return;
	m_impl->call->NotifyCallEnded();
	m_impl->call.Reset();
}

void UniversalMuter::trySetCallName(const std::wstring &callName) {
	if (!m_impl->call)
		return;
	HString name;
	name.Set(callName.c_str());
	m_impl->call->put_ContactName(name.Get());
}

void UniversalMuter::setMuted() {
	if (m_impl->coordinator)
		m_impl->coordinator->NotifyMuted();
}

void UniversalMuter::setUnmuted() {
	if (m_impl->coordinator)
		m_impl->coordinator->NotifyUnmuted();
}
