# vcpkg overlay ports

Our [vcpkg fork](https://github.com/mumble-voip/vcpkg) pins dependency versions for reproducible
builds, but it's synced from upstream [microsoft/vcpkg](https://github.com/microsoft/vcpkg)
manually and irregularly. As of this writing it's missing the `webrtc` port (added upstream on
2026-05-05, after the fork's last sync on 2026-02-15).

Rather than switching the whole build off the pinned fork (and losing `zeroc-ice-mumble`, which
isn't available in upstream vcpkg), the `webrtc` directory here is a copy of upstream's port
recipe, unmodified, taken from
[microsoft/vcpkg@c276a0a](https://github.com/microsoft/vcpkg/commit/c276a0ac) and
[microsoft/vcpkg@cfb5905](https://github.com/microsoft/vcpkg/commit/cfb59059) (MIT licensed, see
[vcpkg's LICENSE.txt](https://github.com/microsoft/vcpkg/blob/master/LICENSE.txt)). Every
dependency it needs (`abseil`, `aom`, `libvpx`, `libyuv`, `libsrtp`, `pffft`, `jsoncpp`,
`vcpkg-tool-gn`, `vcpkg-tool-ninja`, ...) already exists in the mumble-voip fork.

## Usage

Pass this directory to vcpkg alongside your regular (fork) triplet install:

```bash
vcpkg install webrtc --overlay-ports=<path-to-mumble>/vcpkg-overlay-ports --triplet <triplet>
```

## Removing this later

Once `mumble-voip/vcpkg` syncs past 2026-05-05 (check for `ports/webrtc` on its `master` branch),
this overlay is redundant and can be deleted.
