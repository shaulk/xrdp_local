#!/bin/bash

set -e
set -x

ARCH="$1"

mkdir -p output
mkdir -p deps

version=$(git describe --tags | cut -b2-)

xrdp_pkg=xrdp-dmabuf-$version-1-$(uname -m).archlinux.pkg.tar.zst
if ! [ -f deps/$xrdp_pkg ]; then
	curl -L https://github.com/shaulk/xrdp_local_deps/releases/download/v$version/$xrdp_pkg -o deps/$xrdp_pkg
fi

docker buildx build \
	--platform linux/$ARCH \
	-t builder-archlinux-$ARCH \
	-f distros/archlinux/Dockerfile \
	--load \
	.

docker run \
	-i \
	--platform linux/$ARCH \
	-v $PWD/..:/src \
	-v $PWD/deps:/deps \
	-v $PWD/output:/output \
	builder-archlinux-$ARCH \
	bash -c "
		set -x &&
		pacman -U --noconfirm /deps/$xrdp_pkg &&
		mkdir /build &&
		cp -r /src /build/xrdp-local &&
		cp /src/PKGBUILD /build/PKGBUILD &&
		cd /build &&
		tar -czf xrdp-local.tar.gz xrdp-local &&
		cd xrdp-local &&
		useradd -m builder &&
		chown -R builder /build &&
		su - builder -c \"cd /build && makepkg\" &&
		for x in /build/*.pkg.tar.*;
		do
			cp \"\$x\" \"/output/\$(basename \"\$x\" | sed -r \"s/(\\.pkg\\..*)$/.archlinux\\1/\")\" || exit 1
		done
	"
