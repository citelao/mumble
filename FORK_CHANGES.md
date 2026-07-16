# Fork changes

This is a friendly fork of the main Mumble client, designed so we can test a few new features that are currently being built. We are working to merge these changes into main Mumble.

## Changes

* Added WebRTC echo cancellation algorithm.
* Added a vcpkg overlay port for `webrtc`, since our vcpkg fork doesn't have it yet (see
  [`vcpkg-overlay-ports/README.md`](vcpkg-overlay-ports/README.md)).
* Added Universal Mute support on Windows.
* Minor documentation tweak for building on Windows.
* Added this file.