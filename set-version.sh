#!/bin/bash
set -euo pipefail

ROOT_FOLDER="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION_FILE="${ROOT_FOLDER}/VERSION"

usage() {
  cat <<'USAGE'
Usage: ./set-version.sh <version>
Example: ./set-version.sh 1.2.3
USAGE
}

if [[ $# -ne 1 ]]; then
  usage >&2
  exit 1
fi

NEW_VERSION="$1"
if [[ ! "${NEW_VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+([.-][0-9A-Za-z.-]+)?$ ]]; then
  echo "Invalid version: ${NEW_VERSION}" >&2
  echo "Expected format: MAJOR.MINOR.PATCH (optional suffix allowed)" >&2
  exit 1
fi

printf '%s\n' "${NEW_VERSION}" > "${VERSION_FILE}"
printf 'Version set to %s in %s\n' "${NEW_VERSION}" "${VERSION_FILE}"
