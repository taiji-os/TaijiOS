#!/bin/bash
#
# Build script for TaijiOS Android APK
# Requires Android NDK and SDK to be installed
#

set -e

# Configuration
PROJECT_ROOT="/mnt/storage/Projects/TaijiOS"
ANDROID_DIR="${PROJECT_ROOT}/android-port"
APP_DIR="${ANDROID_DIR}/app"
BUILD_DIR="${ANDROID_DIR}/build"

# Android NDK/SDK paths (adjust as needed)
export ANDROID_HOME="${ANDROID_HOME:-/opt/android-sdk}"
export ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT:-${ANDROID_HOME}/ndk/25.2.9519653}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
	echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
	echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
	echo -e "${RED}[ERROR]${NC} $1"
}

# Check prerequisites
check_prereqs() {
	log_info "Checking prerequisites..."

	if [ ! -d "$ANDROID_HOME" ]; then
		log_error "ANDROID_HOME not found at $ANDROID_HOME"
		log_info "Set ANDROID_HOME environment variable or edit this script"
		exit 1
	fi

	if [ ! -d "$ANDROID_NDK_ROOT" ]; then
		log_error "ANDROID_NDK_ROOT not found at $ANDROID_NDK_ROOT"
		log_info "Set ANDROID_NDK_ROOT environment variable or edit this script"
		exit 1
	fi

	# Check for gradle
	if ! command -v gradle &> /dev/null; then
		if [ -f "${ANDROID_DIR}/gradlew" ]; then
			GRADLE="${ANDROID_DIR}/gradlew"
		else
			log_error "Gradle not found and gradlew not present"
			exit 1
		fi
	else
		GRADLE="gradle"
	fi

	log_info "Prerequisites OK"
	log_info "  ANDROID_HOME: $ANDROID_HOME"
	log_info "  ANDROID_NDK_ROOT: $ANDROID_NDK_ROOT"
	log_info "  Gradle: $GRADLE"
}

# Build using Gradle
build_gradle() {
	log_info "Building APK with Gradle..."

	cd "$ANDROID_DIR"

	if [ -f "./gradlew" ]; then
		./gradlew assembleDebug
	else
		gradle assembleDebug
	fi

	if [ $? -eq 0 ]; then
		log_info "Build successful!"
		log_info "APK location: ${APP_DIR}/build/outputs/apk/debug/app-debug.apk"
	else
		log_error "Build failed"
		exit 1
	fi
}

# Clean build
clean_build() {
	log_info "Cleaning build directory..."

	cd "$ANDROID_DIR"

	if [ -f "./gradlew" ]; then
		./gradlew clean
	else
		gradle clean
	fi

	rm -rf "$BUILD_DIR"
}

# Install APK to connected device
install_apk() {
	log_info "Installing APK to connected device..."

	ADB="${ANDROID_HOME}/platform-tools/adb"

	if [ ! -f "$ADB" ]; then
		ADB="adb"
	fi

	"$ADB" install -r "${APP_DIR}/build/outputs/apk/debug/app-debug.apk"

	if [ $? -eq 0 ]; then
		log_info "APK installed successfully!"
		log_info "Launch TaijiOS from your app drawer"
	else
		log_error "Installation failed"
		log_info "Make sure a device is connected via USB or emulator is running"
		exit 1
	fi
}

# Show usage
usage() {
	echo "Usage: $0 [command]"
	echo ""
	echo "Commands:"
	echo "  build    - Build the APK (default)"
	echo "  clean    - Clean build artifacts"
	echo "  install  - Build and install to connected device"
	echo "  help     - Show this help message"
}

# Main
case "${1:-build}" in
	build)
		check_prereqs
		build_gradle
		;;
	clean)
		clean_build
		;;
	install)
		check_prereqs
		build_gradle
		install_apk
		;;
	help|--help|-h)
		usage
		;;
	*)
		log_error "Unknown command: $1"
		usage
		exit 1
		;;
esac
