#!/bin/bash

set -e
set -x

DISTRO="$1"
ARCH="$2"
DISTRO_NAME="${DISTRO%%-*}"
DISTRO_VERSION="${DISTRO#*-}"

case $DISTRO_NAME in
	fedora) DISTRO_SHORTNAME=fc ;;
	*)
		echo "Unknown distro $DISTRO_NAME."
		exit 1
		;;
esac

version=$(git describe --tags | cut -b2-)

mkdir -p output
mkdir -p deps

docker buildx build \
	--platform linux/$ARCH \
	-t builder-$DISTRO_NAME-$DISTRO_VERSION-$ARCH \
	-f distros/$DISTRO_NAME/Dockerfile.$DISTRO_NAME-$DISTRO_VERSION \
	--load \
	.

pkgs=()
for package in xrdp xrdp-devel xrdp-selinux
do
	pkg=$package-$version-1_dmabuf.$DISTRO_SHORTNAME$DISTRO_VERSION.$(docker run --rm --platform linux/$ARCH $DISTRO_NAME:$DISTRO_VERSION bash -c "rpm --eval '%{_arch}'").rpm
	if ! [ -f deps/$pkg ]; then
		curl https://github.com/shaulk/xrdp_local_deps/releases/download/v$version/$pkg -o deps/$pkg
	fi
	pkgs+=($pkg)
done

docker run \
	-i \
	--platform linux/$ARCH \
	-v $PWD/..:/src \
	-v $PWD/deps:/deps \
	-v $PWD/output:/output \
	builder-$DISTRO_NAME-$DISTRO_VERSION-$ARCH \
	bash -c "
		set -x &&
		cd /deps &&
		dnf install -y \
			${pkgs[*]} &&
		mkdir -p /build /root/rpmbuild/SOURCES /root/rpmbuild/SPECS &&
		cp -r /src /build/xrdp-local-$version &&
		cp /src/xrdp_local.spec /root/rpmbuild/SPECS/ &&
		cd /build &&
		tar -czf /root/rpmbuild/SOURCES/xrdp-local-$version.tar.gz xrdp-local-$version &&
		cd ~/rpmbuild/SPECS &&
		rpmbuild -bb xrdp_local.spec &&
		cp ~/rpmbuild/RPMS/*/*.rpm /output/
	"
