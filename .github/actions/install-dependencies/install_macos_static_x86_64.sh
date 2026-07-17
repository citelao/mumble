#!/usr/bin/env bash

set -e
set -x

source "$( dirname "$0" )/common.sh"

verify_required_env_variables_set

# webrtc's build needs a newer libc++ than the default Xcode 15.4 ships (its construct_at
# doesn't support C++20 parenthesized aggregate initialization, which some webrtc code relies
# on). The runner image also has Xcode 16.2 installed side by side; switch to that for this job.
sudo xcode-select -s /Applications/Xcode_16.2.app
echo "DEVELOPER_DIR=/Applications/Xcode_16.2.app/Contents/Developer" >> "$GITHUB_ENV"

brew install coreutils aria2 gnu-tar xz nasm

make_build_env_available "tar.xz"

install_webrtc_overlay "$MUMBLE_VCPKG_TRIPLET" "$GITHUB_WORKSPACE"


# Setup PostgreSQL database for the Mumble tests
# Note: we don't configure MySQL as that's not installed on the Azure runners for macOS
# by default and installing it via homebrew takes forever.

brew install postgresql
brew link postgresql
brew services start postgresql

# Give the database some time to start
sleep 5

configure_database_tables "postgresql"
