#!/bin/bash
set -euo pipefail

readonly SYSTEM_SOURCES_FILE="/etc/apt/sources.list.d/system.sources"
readonly BACKPORTS_SUITE="jammy-backports"
readonly SUITES_PREFIX="Suites:"

system_sources_backup_file=""

cleanup() {
  if [[ -n "${system_sources_backup_file}" && -f "${system_sources_backup_file}" ]]; then
    sudo cp "${system_sources_backup_file}" "${SYSTEM_SOURCES_FILE}"
    rm -f "${system_sources_backup_file}"
  fi
}

create_backup() {
  system_sources_backup_file="$(mktemp)"
  sudo cp "${SYSTEM_SOURCES_FILE}" "${system_sources_backup_file}"
}

has_backports_suite() {
  grep -q "^${SUITES_PREFIX} .*${BACKPORTS_SUITE}" "${SYSTEM_SOURCES_FILE}"
}

disable_backports_suite() {
  sudo sed -i "s/^${SUITES_PREFIX} \\(.*\\) ${BACKPORTS_SUITE}\$/${SUITES_PREFIX} \\1/" "${SYSTEM_SOURCES_FILE}"
}

main() {
  if [[ ! -f "${SYSTEM_SOURCES_FILE}" ]]; then
    echo "Missing APT sources file: ${SYSTEM_SOURCES_FILE}" >&2
    exit 1
  fi

  if ! has_backports_suite; then
    echo "The ${BACKPORTS_SUITE} suite is not enabled in ${SYSTEM_SOURCES_FILE}."
    echo "Running the standard dependency installer."
    exec "$(dirname "$0")/deps.sh"
  fi

  trap cleanup EXIT

  create_backup
  disable_backports_suite

  sudo apt-get update
  "$(dirname "$0")/deps.sh"
}

main
