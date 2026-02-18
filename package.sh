#!/bin/bash
set -euo pipefail

ROOT_FOLDER="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="Release"
BUILD_FOLDER="${ROOT_FOLDER}/build-package-${BUILD_TYPE,,}"
OUTPUT_FOLDER="${ROOT_FOLDER}/dist"
VERSION="$(tr -d '[:space:]' < "${ROOT_FOLDER}/VERSION")"

cmake -S "${ROOT_FOLDER}" \
  -B "${BUILD_FOLDER}" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

cmake --build "${BUILD_FOLDER}"

mkdir -p "${OUTPUT_FOLDER}"
rm -f "${BUILD_FOLDER}/galman_"*.deb "${BUILD_FOLDER}/galman-"*.tar.gz "${BUILD_FOLDER}/galman-"*.sh "${BUILD_FOLDER}/galman-"*.rpm 2>/dev/null || true
(
  cd "${BUILD_FOLDER}"
  cpack
)

rm -f "${OUTPUT_FOLDER}/galman_"*.deb "${OUTPUT_FOLDER}/galman-"*.tar.gz "${OUTPUT_FOLDER}/galman-"*.sh "${OUTPUT_FOLDER}/galman-"*.rpm 2>/dev/null || true

find "${BUILD_FOLDER}" -maxdepth 1 -type f \
  \( -name "galman_${VERSION}_*.deb" -o -name "galman-${VERSION}-*.tar.gz" -o -name "galman-${VERSION}-*.sh" -o -name "galman-${VERSION}-*.rpm" \) \
  -exec cp -f {} "${OUTPUT_FOLDER}/" \;

echo
printf 'Packages generated in %s\n' "${OUTPUT_FOLDER}"
find "${OUTPUT_FOLDER}" -maxdepth 1 -type f \( -name "*.deb" -o -name "*.tar.gz" -o -name "*.sh" -o -name "*.rpm" \) -print | sort
