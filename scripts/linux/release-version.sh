#!/bin/bash
set -euo pipefail

SCRIPT_FOLDER="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_FOLDER="$(cd "${SCRIPT_FOLDER}/../.." && pwd)"
VERSION_FILE="${PROJECT_ROOT_FOLDER}/VERSION"

EXIT_SUCCESS=0
EXIT_FAILURE=1
VERSION_PATTERN='^[0-9]+\.[0-9]+\.[0-9]+([.-][0-9A-Za-z.-]+)?$'

usage() {
  cat <<'USAGE'
Usage: ./release-version.sh

Interactive flow:
1. Show current version
2. Prompt for new version
3. Allow cancellation
4. Update tracked text files containing the current version
5. Create commit and tag v<version>
USAGE
}

ensure_git_repository() {
  if ! git -C "${PROJECT_ROOT_FOLDER}" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    echo "This script must be run inside a Git repository." >&2
    exit "${EXIT_FAILURE}"
  fi
}

ensure_clean_worktree() {
  if [[ -n "$(git -C "${PROJECT_ROOT_FOLDER}" status --short)" ]]; then
    echo "Git worktree is not clean. Commit or stash existing changes before releasing a version." >&2
    exit "${EXIT_FAILURE}"
  fi
}

ensure_version_file_exists() {
  if [[ ! -f "${VERSION_FILE}" ]]; then
    echo "Version file not found: ${VERSION_FILE}" >&2
    exit "${EXIT_FAILURE}"
  fi
}

read_current_version() {
  tr -d '[:space:]' < "${VERSION_FILE}"
}

prompt_new_version() {
  local current_version="$1"
  local candidate_version=""

  printf 'Current version: %s\n' "${current_version}" >&2
  read -r -p "New version (empty to cancel): " candidate_version

  if [[ -z "${candidate_version}" ]]; then
    echo "Operation cancelled." >&2
    exit "${EXIT_SUCCESS}"
  fi

  if [[ "${candidate_version}" == "cancel" ]]; then
    echo "Operation cancelled." >&2
    exit "${EXIT_SUCCESS}"
  fi

  if [[ ! "${candidate_version}" =~ ${VERSION_PATTERN} ]]; then
    echo "Invalid version: ${candidate_version}" >&2
    echo "Expected format: MAJOR.MINOR.PATCH (optional suffix allowed)" >&2
    exit "${EXIT_FAILURE}"
  fi

  if [[ "${candidate_version}" == "${current_version}" ]]; then
    echo "New version matches the current version." >&2
    exit "${EXIT_FAILURE}"
  fi

  printf '%s\n' "${candidate_version}"
}

ensure_tag_does_not_exist() {
  local new_version="$1"
  local tag_name="v${new_version}"

  if git -C "${PROJECT_ROOT_FOLDER}" rev-parse --verify --quiet "refs/tags/${tag_name}" >/dev/null; then
    echo "Tag already exists: ${tag_name}" >&2
    exit "${EXIT_FAILURE}"
  fi
}

update_version_in_tracked_text_files() {
  local current_version="$1"
  local new_version="$2"
  local file_path=""

  while IFS= read -r -d '' file_path; do
    if ! grep -Iq . "${PROJECT_ROOT_FOLDER}/${file_path}"; then
      continue
    fi

    if ! grep -Fq "${current_version}" "${PROJECT_ROOT_FOLDER}/${file_path}"; then
      continue
    fi

    OLD_VERSION="${current_version}" \
    NEW_VERSION="${new_version}" \
      perl -0pi -e 's/\Q$ENV{OLD_VERSION}\E/$ENV{NEW_VERSION}/g' "${PROJECT_ROOT_FOLDER}/${file_path}"
  done < <(git -C "${PROJECT_ROOT_FOLDER}" ls-files -z)
}

ensure_version_changed() {
  if git -C "${PROJECT_ROOT_FOLDER}" diff --quiet --exit-code; then
    echo "No file was updated. Aborting release." >&2
    exit "${EXIT_FAILURE}"
  fi
}

create_release_commit_and_tag() {
  local new_version="$1"
  local tag_name="v${new_version}"

  git -C "${PROJECT_ROOT_FOLDER}" add -A
  git -C "${PROJECT_ROOT_FOLDER}" commit -m "Release ${new_version}"
  git -C "${PROJECT_ROOT_FOLDER}" tag "${tag_name}"

  printf 'Created commit and tag %s\n' "${tag_name}"
}

main() {
  if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit "${EXIT_SUCCESS}"
  fi

  if [[ $# -ne 0 ]]; then
    usage >&2
    exit "${EXIT_FAILURE}"
  fi

  ensure_git_repository
  ensure_clean_worktree
  ensure_version_file_exists

  local current_version=""
  local new_version=""

  current_version="$(read_current_version)"
  new_version="$(prompt_new_version "${current_version}")"

  ensure_tag_does_not_exist "${new_version}"
  update_version_in_tracked_text_files "${current_version}" "${new_version}"
  ensure_version_changed
  create_release_commit_and_tag "${new_version}"
}

main "$@"
