#!/bin/bash
set -euo pipefail

SCRIPT_FOLDER="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_FOLDER="$(cd "${SCRIPT_FOLDER}/../.." && pwd)"

BUILD_TYPE="Release"
BUILD_FOLDER="${PROJECT_ROOT_FOLDER}/build-${BUILD_TYPE,,}"
BINARY_PATH="${BUILD_FOLDER}/galman"
ICON_PATH="${PROJECT_ROOT_FOLDER}/qml/Assets/Galman.png"
DESKTOP_FILE="${HOME}/.local/share/applications/galman.desktop"

if [ ! -f "${BINARY_PATH}" ]; then
  echo "Release binary not found at ${BINARY_PATH}. Build first with: ./scripts/linux/build.sh --release" >&2
  exit 1
fi

mkdir -p "${HOME}/.local/share/applications"

cat > "${DESKTOP_FILE}" <<EOF
[Desktop Entry]
Type=Application
Name=Galman
Comment=Image gallery manager
Exec=${BINARY_PATH}
Icon=${ICON_PATH}
Terminal=false
Categories=Graphics;Photography;Qt;
EOF

echo "Desktop shortcut installed at ${DESKTOP_FILE}"
