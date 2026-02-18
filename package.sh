#!/bin/bash
set -euo pipefail

ROOT_FOLDER="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="Release"
BUILD_FOLDER="${ROOT_FOLDER}/build-package-${BUILD_TYPE,,}"
OUTPUT_FOLDER="${ROOT_FOLDER}/dist"
PACKAGE_VERSION="${1:-0.1.0}"

cmake -S "${ROOT_FOLDER}" \
  -B "${BUILD_FOLDER}" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DGALMAN_PACKAGE_VERSION="${PACKAGE_VERSION}"

cmake --build "${BUILD_FOLDER}"

mkdir -p "${OUTPUT_FOLDER}"
(
  cd "${BUILD_FOLDER}"
  cpack
)

find "${BUILD_FOLDER}" -maxdepth 1 -type f \
  \( -name "*.deb" -o -name "*.tar.gz" -o -name "*.sh" -o -name "*.rpm" \) \
  -exec cp -f {} "${OUTPUT_FOLDER}/" \;

echo
printf 'Packages generated in %s\n' "${OUTPUT_FOLDER}"
find "${OUTPUT_FOLDER}" -maxdepth 1 -type f \( -name "*.deb" -o -name "*.tar.gz" -o -name "*.sh" -o -name "*.rpm" \) -print | sort
