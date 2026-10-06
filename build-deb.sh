#!/usr/bin/env bash
#
# Build the libudt0 and libudt-dev .deb files for Debian 13 from this tree:
#
#   ./build-deb.sh                  # on a Debian 13 build host; -> out/deb/
#   USE_SBUILD=1 ./build-deb.sh     # in an sbuild unshare chroot
#
# The version is the top-level VERSION file's. debian/changelog is written
# here from it, so it never goes stale.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION="$(tr -d '[:space:]' <"${SCRIPT_DIR}/VERSION")"
BUILD_ROOT="${BUILD_ROOT:-${HOME}/udt-deb-build}"
OUT_DIR="${SCRIPT_DIR}/out/deb"

die() { printf 'udt-deb: %s\n' "$*" >&2; exit 1; }

grep -q '^13' /etc/debian_version 2>/dev/null \
    || die "this builder targets Debian 13, found: $(cat /etc/debian_version 2>/dev/null || echo unknown)"
[[ -n "${VERSION}" ]] || die "no version in ${SCRIPT_DIR}/VERSION"

SRC_DIR="${BUILD_ROOT}/udt-${VERSION}"
rm -rf "${SRC_DIR}" "${BUILD_ROOT}"/udt_"${VERSION}"* "${BUILD_ROOT}"/libudt*_"${VERSION}"*
mkdir -p "${SRC_DIR}"
cp -a "${SCRIPT_DIR}"/{VERSION,Makefile,LICENSE,README.md,src,tests,udt-doc,debian} "${SRC_DIR}/"
cat >"${SRC_DIR}/debian/changelog" <<EOC
udt (${VERSION}) trixie; urgency=medium

  * UDT ${VERSION} from the dorkbox tree for Debian 13: a Makefile that
    builds libudt.so.0, multiarch paths, a loopback test run in the build.

 -- Dmitry Musatov <dm@dgma.io>  $(date -R)
EOC

echo "==> Building"
cd "${SRC_DIR}"
if [[ -n "${USE_SBUILD:-}" ]]; then
    dpkg-source -b .
    sbuild --chroot-mode=unshare --dist "$(. /etc/os-release; echo "${VERSION_CODENAME}")" \
        --no-run-lintian --no-source --arch-any --no-arch-all \
        "../udt_${VERSION}.dsc"
else
    dpkg-buildpackage -us -uc -b
fi
mkdir -p "${OUT_DIR}"
rm -f "${OUT_DIR}"/*.deb
for pkg in libudt0 libudt-dev; do
    find "${BUILD_ROOT}" -maxdepth 2 -name "${pkg}_${VERSION}_*.deb" -exec cp -v {} "${OUT_DIR}/" \;
    ls "${OUT_DIR}/${pkg}_${VERSION}"_*.deb >/dev/null 2>&1 \
        || die "no ${pkg}_${VERSION} .deb under ${BUILD_ROOT}"
done
echo "==> Done:"
ls -l "${OUT_DIR}"
