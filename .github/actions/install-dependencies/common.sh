# Copyright The Mumble Developers. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file at the root of the
# Mumble source tree or at <https://www.mumble.info/LICENSE>.

set -e
set -x

verify_required_env_variables_set() {
	if [[ -z "$MUMBLE_ENVIRONMENT_SOURCE" ]]; then
		echo "MUMBLE_ENVIRONMENT_SOURCE not set!" 1>&2
		exit 1
	fi

	if [[ -z "$MUMBLE_ENVIRONMENT_VERSION" ]]; then
		echo "MUMBLE_ENVIRONMENT_VERSION not set!" 1>&2
		exit 1
	fi

	if [[ -z "$MUMBLE_ENVIRONMENT_DIR" ]]; then
		echo "MUMBLE_ENVIRONMENT_DIR not set!" 1>&2
		exit 1
	fi
}

extract_with_progress() {
	local fromFile="$1"
	local targetDir="$2"

	if [ -z "$fromFile" ]; then
		echo "[ERROR]: Missing argument"
		exit 1
	fi

	if [ -z "$targetDir" ]; then
		targetDir="."
	fi

	echo "Extracting from \"$fromFile\" to \"$targetDir\""
	echo ""

	# Make targetDir an absolute path
	targetDir="$( realpath "$targetDir" )"

	# Use gtar and gwc if available (for MacOS compatibility)
	tar_exe="tar"
	if [ -x "$(command -v gtar)" ]; then
		tar_exe="gtar"
	fi
	wc_exe="wc"
	if [ -x "$(command -v gwc)" ]; then
		wc_exe="gwc"
	fi

	tmp_dir="__extract_root__"
	mkdir "$tmp_dir"

	if [[ "$fromFile" = *.7z || "$fromFile"  = *.zip ]]; then
		extract_cmd=( 7z x "$fromFile" -o"$tmp_dir" )

		summary="$( 7z l "$fromFile" | tail -n 1 )"
		fromSize="$( echo "$summary" | tr -s ' ' | cut -d ' ' -f 4 )"
		toSize="$( echo "$summary" | tr -s ' ' | cut -d ' ' -f 3 )"
	else
		# Get sizes in bytes
		fromSize=$(xz --robot --list "$fromFile" | tail -n -1 | cut -f 4)
		toSize=$(xz --robot --list "$fromFile" | tail -n -1 | cut -f 5)

		steps=100
		checkPointStep=$(expr "$toSize" / 1000 / "$steps" )

		extract_cmd=( "$tar_exe" -x --record-size=1K --checkpoint="$checkPointStep" --checkpoint-action="echo=%u / $toSize" --file "$fromFile" --directory "$tmp_dir" )
	fi

	# Convert sizes to KB
	local toSizeKB=$(expr "$toSize" / 1000)
	local fromSizeKB=$(expr "$fromSize" / 1000)

	echo "Compressed size:   $fromSizeKB KB"
	echo "Uncompressed size: $toSizeKB KB"


	echo ""

	"${extract_cmd[@]}"

	local num_files="$( ls -Al "$tmp_dir" | tail -n +2 | $wc_exe -l )"

	if [[ ! -d "$targetDir" ]]; then
		mkdir "$targetDir"
	fi

	if [[ "$num_files" = 1 && -d "$tmp_dir/$( ls "$tmp_dir" )" ]]; then
		# Skip top-level directory
		pushd "$(pwd)"
		cd "$tmp_dir"/*
		mv * "$targetDir"
		mv .* "$targetDir" 2> /dev/null || true
		popd
	else
		# Move all files
		mv "$tmp_dir"/* "$targetDir"
		mv "$tmp_dir"/.* "$targetDir" 2> /dev/null || true
	fi

	rm -rf "$tmp_dir"
}

make_build_env_available() {
	local env_file_extension="$1"

	if [[ -z "$env_file_extension" ]]; then
		echo "No file extension provided" 1>&2
		exit 1
	fi

	local env_dir="$MUMBLE_ENVIRONMENT_DIR"

	if [[ -d "$env_dir" && -n "$( ls -A "$env_dir" )" ]]; then
		echo "Environment is cached"
	else
		echo "Environment not cached -> downloading now"

		local env_archive="$MUMBLE_ENVIRONMENT_VERSION.$env_file_extension"

		aria2c "$MUMBLE_ENVIRONMENT_SOURCE/$env_archive" --out="$env_archive"

		echo "Extracting archive..."
		if [[ ! -d "$env_dir" ]]; then
			mkdir -p "$env_dir"
		fi

		extract_with_progress "$env_archive" "$env_dir"

		if [[ ! -d "$env_dir" || -z "$( ls -A "$env_dir" )" ]]; then
			echo "Environment did not follow expected form" 1>&2
			ls -al "$env_dir"
			exit 1
		fi

		chmod +x "$env_dir/installed/$MUMBLE_VCPKG_TRIPLET/tools/Ice/slice2cpp"
	fi
}

# The pre-built environment archives from mumble-voip/vcpkg releases only contain the
# "installed" output (plus the vcpkg binary and CMake toolchain scripts) - they don't include
# the ports/triplets trees, so `vcpkg install` can't resolve any new package against
# them as-is. To install `webrtc` (missing from our vcpkg fork as of its 2026-02-15 sync, see
# vcpkg-overlay-ports/README.md) we fetch just those trees from the fork at the exact commit
# the environment was built from, then run a live vcpkg install using our overlay port,
# reusing the environment's already-built packages instead of rebuilding everything.
install_webrtc_overlay() {
	local triplet="$1"
	local repo_root="$2"

	if [[ -z "$triplet" || -z "$repo_root" ]]; then
		echo "install_webrtc_overlay: missing triplet or repo_root argument" 1>&2
		exit 1
	fi

	local env_dir="$MUMBLE_ENVIRONMENT_DIR"

	if [[ ! -d "$env_dir/ports" ]]; then
		echo "Fetching vcpkg port recipes at $MUMBLE_ENVIRONMENT_COMMIT"

		local vcpkg_src_dir
		vcpkg_src_dir="$(mktemp -d)"

		aria2c "https://codeload.github.com/mumble-voip/vcpkg/tar.gz/$MUMBLE_ENVIRONMENT_COMMIT" --out="vcpkg-src.tar.gz" --dir="$vcpkg_src_dir"
		tar -xzf "$vcpkg_src_dir/vcpkg-src.tar.gz" -C "$vcpkg_src_dir" --strip-components=1

		# The environment archive only ships a partial tree (installed/, the vcpkg binary,
		# and just enough of scripts/ for CMake's toolchain file) - vcpkg itself needs the
		# rest (ports/, triplets/, the full scripts/ directory, even top-level files like
		# LICENSE.txt that its portfiles install) to actually run a new install. Merge the
		# full fetched tree in without clobbering what's already there (the prebuilt
		# installed/ output and vcpkg binary). BSD cp (macOS) exits 1 whenever -n skips an
		# existing file, unlike GNU cp - that's expected here, so don't let it abort the script.
		cp -Rn "$vcpkg_src_dir/." "$env_dir/" || true

		rm -rf "$vcpkg_src_dir"
	fi

	local vcpkg_exe="$env_dir/vcpkg"
	if [[ ! -x "$vcpkg_exe" ]]; then
		vcpkg_exe="$env_dir/vcpkg.exe"
	fi

	if [[ ! -x "$vcpkg_exe" ]]; then
		echo "No vcpkg executable found in $env_dir" 1>&2
		exit 1
	fi

	"$vcpkg_exe" install webrtc \
		--overlay-ports="$repo_root/vcpkg-overlay-ports" \
		--triplet "$triplet"
}

configure_database_tables() {
	if [[ -x "$( which sudo )" ]]; then
		sudo_cmd="sudo"
	else
		sudo_cmd=""
	fi

	while [[ "$#" -gt 0 ]]; do
		case "$1" in
			"mysql")
				local sql_statements='CREATE DATABASE `mumble_test-db`;'
				sql_statements+="CREATE USER 'mumble_test-user'@'localhost' IDENTIFIED BY 'MumbleTestPassword';"
				sql_statements+="GRANT ALL PRIVILEGES ON \`mumble_test-db\`.* TO 'mumble_test-user'@'localhost';"

				if $sudo_cmd mysql --user=root -e "SELECT 1" 2> /dev/null; then
					# Passwordless
					mysql_cmd=( $sudo_cmd mysql --user=root )
				else
					mysql_cmd=( $sudo_cmd mysql --user=root --password="root" )
				fi

				echo "$sql_statements" | "${mysql_cmd[@]}"
				;;

			"postgresql")
				local sql_statements='CREATE DATABASE "mumble_test-db";'
				sql_statements+="CREATE USER \"mumble_test-user\" ENCRYPTED PASSWORD 'MumbleTestPassword';"
				sql_statements+='ALTER DATABASE "mumble_test-db" OWNER TO "mumble_test-user";'

				if [[ -n "$sudo_cmd" ]] && id -u postgres > /dev/null 1>&1; then
					# User postgres exists and we can use sudo to execute commands as that user
					psql_cmd=( "$sudo_cmd" -u postgres psql )
				else
					psql_cmd=( psql -d postgres )
				fi

				echo "$sql_statements" | "${psql_cmd[@]}"
				;;

			*)
				echo "Unsupported database '$1'" 1>&2
				exit 1
				;;
		esac

		shift
	done
}
