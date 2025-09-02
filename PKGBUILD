# Maintainer: Shaul Kremer <shaulk@users.noreply.github.com>

pkgname=xrdp-local
pkgver=0.10.4.1
pkgrel=1
pkgdesc="Local client for xrdp providing seamless switching between local and RDP connections - core"
url="https://github.com/shaulk/xrdp_local"
arch=(x86_64 aarch64)
license=('Apache-2.0')
makedepends=('cmake' 'argparse' 'gcc' 'libegl' 'xrdp-dmabuf' 'qt6-base')
depends=('libegl' 'xrdp-dmabuf' 'qt6-base')
source=("xrdp-local.tar.gz")
sha256sums=('SKIP')

prepare() {
  true
}

build() {
  cd "${pkgname}"
  cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
  cd build
  make -j$(nproc)
}

package() {
  cd "${pkgname}"/build
  make install DESTDIR="${pkgdir}"
}
