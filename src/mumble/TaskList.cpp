// Copyright The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "TaskList.h"

#include "MumbleApplication.h"

#include "win.h"

#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>

// We disable clang-format for these includes as apparently the order in which they are
// included is important.
// clang-format off
#include <shlobj.h>
#include <shobjidl.h>
#include <propkey.h>
#include <propvarutil.h>
// clang-format on

#include <winrt/windows.applicationmodel.calls.h>

extern bool bIsWin7;

namespace {
void test()
{
	const auto coordinator = winrt::Windows::ApplicationModel::Calls::VoipCallCoordinator::GetDefault();
	const auto call =
		coordinator.RequestNewOutgoingCall(L"context_link", L"contact_name", L"Mumble",
										   winrt::Windows::ApplicationModel::Calls::VoipPhoneCallMedia::Audio);
	call.NotifyCallActive();
	//coordinator.NotifyMuted();
	call.NotifyCallEnded();
}
}

void TaskList::addToRecentList(QString name, QString user, QString host, int port) {
	if (!bIsWin7)
		return;

	HRESULT hr;
	IShellLink *link   = nullptr;
	IPropertyStore *ps = nullptr;
	PROPVARIANT pt;

	hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, __uuidof(IShellLink),
						  reinterpret_cast< void ** >(&link));
	if (!link || FAILED(hr))
		return;

	QUrl url;
	url.setScheme(QLatin1String("mumble"));
	url.setUserName(user);
	url.setHost(host);
	url.setPort(port);

	QUrlQuery query;
	query.addQueryItem(QLatin1String("title"), name);
	query.addQueryItem(QLatin1String("version"), QLatin1String("1.2.0"));
	url.setQuery(query);

	QSettings settings(QLatin1String("HKEY_CLASSES_ROOT"), QSettings::NativeFormat);

	QString app = settings.value(QLatin1String("mumble/DefaultIcon/.")).toString();
	if (app.isEmpty() || !QFileInfo(app).exists())
		app = QCoreApplication::applicationFilePath();

	link->SetPath(app.toStdWString().c_str());
	link->SetArguments(QString::fromLatin1(url.toEncoded()).toStdWString().c_str());

	hr = link->QueryInterface(__uuidof(IPropertyStore), reinterpret_cast< void ** >(&ps));
	if (FAILED(hr)) {
		qFatal("TaskList: Failed to get property store");
		goto cleanup;
	}

	InitPropVariantFromString(name.toStdWString().c_str(), &pt);
	hr = ps->SetValue(PKEY_Title, pt);
	PropVariantClear(&pt);

	if (FAILED(hr)) {
		qFatal("TaskList: Failed to set title");
		goto cleanup;
	}

	hr = ps->Commit();
	if (FAILED(hr)) {
		qFatal("TaskList: Failed commit");
		goto cleanup;
	}

	SHAddToRecentDocs(SHARD_LINK, link);

cleanup:
	if (ps)
		ps->Release();
	if (link)
		link->Release();
}
