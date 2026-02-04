#!/bin/bash
#
# Release script for TaijiOS Android APK
# Builds, signs, and packages a release APK
#

set -e

VERSION="0.1.0"
VERSION_CODE="1"
PACKAGE="org.taijos.os"
PROJECT_ROOT="/mnt/storage/Projects/TaijiOS"
ANDROID_DIR="${PROJECT_ROOT}/android-port"
BUILD_DIR="${ANDROID_DIR}/app/build/outputs/apk"
RELEASE_DIR="${ANDROID_DIR}/release"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() {
	echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
	echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
	echo -e "${RED}[ERROR]${NC} $1"
}

# Clean previous builds
clean_build() {
	log_info "Cleaning previous builds..."
	cd "$ANDROID_DIR"
	if [ -f "./gradlew" ]; then
		./gradlew clean
	fi
	rm -rf "$RELEASE_DIR"
	mkdir -p "$RELEASE_DIR"
}

# Build release APK
build_release() {
	log_info "Building release APK..."
	cd "$ANDROID_DIR"

	if [ -f "./gradlew" ]; then
		./gradlew assembleRelease
	else
		gradle assembleRelease
	fi

	if [ ! -f "${BUILD_DIR}/release/app-release-unsigned.apk" ]; then
		log_error "Release APK not found"
		exit 1
	fi

	log_info "APK built successfully"
}

# Sign APK
sign_apk() {
	log_info "Signing APK..."

	# Check for keystore
	if [ ! -f "$ANDROID_DIR/release.keystore" ]; then
		log_warn "Keystore not found, generating new one..."
		keytool -genkey -v \
			-keystore "$ANDROID_DIR/release.keystore" \
			-alias taijos \
			-keyalg RSA \
			-keysize 2048 \
			-validity 10000 \
			-dname "CN=TaijiOS, OU=Development, O=TaijiOS, L=Unknown, ST=Unknown, C=US" \
			-storepass taijos123 \
			-keypass taijos123
	fi

	# Copy unsigned APK
	cp "${BUILD_DIR}/release/app-release-unsigned.apk" "$RELEASE_DIR/taijos-${VERSION}-unsigned.apk"

	# Sign APK
	jarsigner -verbose \
		-sigalg SHA1withRSA \
		-digestalg SHA1 \
		-keystore "$ANDROID_DIR/release.keystore" \
		-storepass taijos123 \
		-keypass taijos123 \
		"$RELEASE_DIR/taijos-${VERSION}-unsigned.apk" \
		taijos

	# Align APK
	zipalign -v 4 \
		"$RELEASE_DIR/taijos-${VERSION}-unsigned.apk" \
		"$RELEASE_DIR/TaijiOS-${VERSION}.apk"

	log_info "Signed APK: $RELEASE_DIR/TaijiOS-${VERSION}.apk"
}

# Generate checksums
generate_checksums() {
	log_info "Generating checksums..."
	cd "$RELEASE_DIR"

	sha256sum "TaijiOS-${VERSION}.apk" > "TaijiOS-${VERSION}.apk.sha256"
	md5sum "TaijiOS-${VERSION}.apk" > "TaijiOS-${VERSION}.apk.md5"

	log_info "Checksums generated"
}

# Create release notes
create_release_notes() {
	cat > "$RELEASE_DIR/RELEASE_NOTES.md" <<EOF
# TaijiOS ${VERSION} - Android Release

## What's New

This is the initial Android release of TaijiOS.

## Features

- Native ARM64 execution
- Window Manager with touch support
- Kryon UI framework
- OpenGL ES hardware-accelerated graphics
- Dis VM (portable bytecode interpreter)

## Requirements

- Android 5.0 (API 21) or later
- ARM64 device (armeabi-v7a also available)
- OpenGL ES 3.0 support

## Installation

1. Download \`TaijiOS-${VERSION}.apk\`
2. Enable "Install from unknown sources" in Android settings
3. Open the APK file to install

## Verification

Verify the download using the provided checksums:
\`\`\`bash
sha256sum -c TaijiOS-${VERSION}.apk.sha256
\`\`\`

## Source Code

Source code available at: https://github.com/yourname/TaijiOS

## Bug Reports

Please report issues at: https://github.com/yourname/TaijiOS/issues

## License

GPL-2.0-or-later
EOF

	log_info "Release notes created"
}

# Main
case "${1:-all}" in
	clean)
		clean_build
		;;
	build)
		clean_build
		build_release
		;;
	sign)
		build_release
		sign_apk
		;;
	checksums)
		generate_checksums
		;;
	all)
		clean_build
		build_release
		sign_apk
		generate_checksums
		create_release_notes

		log_info "Release complete!"
		log_info "Output directory: $RELEASE_DIR"
		log_info "APK: TaijiOS-${VERSION}.apk"
		;;
	*)
		log_error "Unknown command: $1"
		echo "Usage: $0 [clean|build|sign|checksums|all]"
		exit 1
		;;
esac
