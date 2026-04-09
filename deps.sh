#!/bin/bash
set -euo pipefail

APT_PACKAGES=(
  build-essential
  cmake
  ninja-build
  git
  libgl-dev
  qt6-base-dev
  qt6-base-dev-tools
  qt6-declarative-dev
  qt6-declarative-dev-tools
  qt6-multimedia-dev
  qt6-image-formats-plugins
  qt6-qmltooling-plugins
  qt6-tools-dev
  qt6-tools-dev-tools
  linguist-qt6
  qt6-l10n-tools
  libqt6shadertools6-dev
  qt6-shader-baker
  qt6-wayland
  qml6-module-qtquick
  qml6-module-qtquick-controls
  qml6-module-qtquick-layouts
  qml6-module-qtqml-models
  qml6-module-qtqml-workerscript
  qml6-module-qtquick-templates
  qml6-module-qtquick-window
  qml6-module-qtmultimedia
)

DNF_PACKAGES=(
  gcc-c++
  cmake
  ninja-build
  git
  mesa-libGL-devel
  qt6-qtbase-devel
  qt6-qtdeclarative-devel
  qt6-qtmultimedia-devel
  qt6-qtimageformats
  qt6-qttools-devel
  qt6-qtshadertools-devel
  qt6-qtwayland-devel
)

PACMAN_PACKAGES=(
  base-devel
  cmake
  ninja
  git
  mesa
  qt6-base
  qt6-declarative
  qt6-multimedia
  qt6-imageformats
  qt6-tools
  qt6-shadertools
  qt6-wayland
)

install_with_apt() {
  sudo apt-get update
  sudo apt-get install "${APT_PACKAGES[@]}"
}

install_with_dnf() {
  sudo dnf install "${DNF_PACKAGES[@]}"
}

install_with_pacman() {
  sudo pacman -S --needed "${PACMAN_PACKAGES[@]}"
}

main() {
  if command -v apt-get >/dev/null 2>&1; then
    install_with_apt
    return
  fi

  if command -v dnf >/dev/null 2>&1; then
    install_with_dnf
    return
  fi

  if command -v pacman >/dev/null 2>&1; then
    install_with_pacman
    return
  fi

  echo "Unsupported package manager. Supported package managers: apt-get, dnf, pacman." >&2
  exit 1
}

main
