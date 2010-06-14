#!/bin/bash -p

# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# usage: dmgdiffer.sh old_dmg new_dmg patch_dmg
#
# dmgdiffer creates a disk image containing a binary update able to patch
# a product originally distributed in old_dmg to the version in new_dmg. Much
# of this script is generic, but the make_patch_fs function is specific to
# a product: in this case, Google Chrome.
#
# This script operates by mounting old_dmg and new_dmg, creating a new
# filesystem structure containing dirpatches generated by dirdiffer and
# goobsdiff (which should be located in the same directory as this script),
# and producing a disk image from that structure.
#
# The Chrome make_patch_fs function produces an disk image that is able to
# update a single old version on any Keystone channel to a new version on a
# specific Keystone channel (the Keystone channel associated with new_dmg).
# Chrome's updates are split into two dirpatches: one updates the old
# versioned directory to the new one, and the other updates the remainder of
# the application. The versioned directory is split out from the rest because
# it contains the bulk of the application and its name changes from version to
# version, and dirdiffer/dirpatcher do not directly handle name changes. This
# approach also allows the versioned directory dirpatch to be applied in-place
# in most cases during an update, rather than relying on a temporary
# directory. In order to allow a single update dmg to apply to an old version
# on any Keystone channel, several small files are never distributed as diffs,
# and only as full (possibly compressed) versions of the new files. These
# files include the outer application's Info.plist which contains Keystone
# channel information, and anything created or modified by code-signinig the
# outer application.
#
# Application of update disk images produced by this script is
# product-specific. With updates managed by Keystone, the update disk images
# can contain a .keystone_install script that is able to locate and update
# the installed product.
#
# Exit codes:
#  0  OK
#  1  Unknown failure
#  2  Incorrect number of parameters
#  3  Input disk images do not exist
#  4  Output disk image already exists
#  5  Parent of output directory does not exist or is not a directory
#  6  Could not mount old_dmg
#  7  Could not mount new_dmg
#  8  Could not create temporary patch filesystem directory
#  9  Could not create disk image
# 10  Could not read old application data
# 11  Could not read new application data
# 12  Old or new application sanity check failure
# 13  Could not write the patch
#
# Exit codes in the range 21-40 are mapped to codes 1-20 as returned by the
# first dirdiffer invocation. Codes 41-60 are mapped to codes 1-20 as returned
# by the second.

set -eu

# Environment sanitization. Set a known-safe PATH. Clear environment variables
# that might impact the interpreter's operation. The |bash -p| invocation
# on the #! line takes the bite out of BASH_ENV, ENV, and SHELLOPTS (among
# other features), but clearing them here ensures that they won't impact any
# shell scripts used as utility programs. SHELLOPTS is read-only and can't be
# unset, only unexported.
export PATH="/usr/bin:/bin:/usr/sbin:/sbin"
unset BASH_ENV CDPATH ENV GLOBIGNORE IFS POSIXLY_CORRECT
export -n SHELLOPTS

ME="$(basename "${0}")"
readonly ME
SCRIPT_DIR="$(dirname "${0}")"
readonly SCRIPT_DIR
readonly DIRDIFFER="${SCRIPT_DIR}/dirdiffer.sh"
readonly PKG_DMG="${SCRIPT_DIR}/pkg-dmg"

err() {
  local error="${1}"

  echo "${ME}: ${error}" >& 2
}

declare -a g_cleanup g_cleanup_mount_points
cleanup() {
  local status=${?}

  trap - EXIT
  trap '' HUP INT QUIT TERM

  if [[ ${status} -ge 128 ]]; then
    err "Caught signal $((${status} - 128))"
  fi

  if [[ "${#g_cleanup_mount_points[@]}" -gt 0 ]]; then
    local mount_point
    for mount_point in "${g_cleanup_mount_points[@]}"; do
      hdiutil detach "${mount_point}" -force >& /dev/null || true
    done
  fi

  if [[ "${#g_cleanup[@]}" -gt 0 ]]; then
    rm -rf "${g_cleanup[@]}"
  fi

  exit ${status}
}

mount_dmg() {
  local dmg="${1}"
  local mount_point="${2}"

  if ! hdiutil attach "${1}" -mountpoint "${2}" \
                             -nobrowse -owners off > /dev/null; then
    # set -e is in effect. return ${?} so that the caller can check the return
    # code if desired, perhaps to print a more useful error message or to exit
    # with a more precise status than would be possible here.
    return ${?}
  fi
}

# make_patch_fs is responsible for comparing the old and new disk images
# mounted at old_fs and new_fs, respectively, and populating patch_fs with the
# contents of what will become a disk image able to update old_fs to new_fs.
# It then outputs a string which will be used as the volume name of the
# patch_dmg.
#
# The entire patch contents are placed into a .patch directory to hide them
# from ordinary view. The disk image will be given a volume name like
# "Google Chrome 5.0.375.55-5.0.375.70" as an identifying aide, although
# uniqueness is not important and users will never interact directly with
# them.
make_patch_fs() {
  local old_fs="${1}"
  local new_fs="${2}"
  local patch_fs="${3}"

  readonly PRODUCT_NAME="Google Chrome"
  readonly APP_NAME="${PRODUCT_NAME}.app"
  readonly APP_NAME_RE="${PRODUCT_NAME}\\.app"
  readonly APP_PLIST="Contents/Info"
  readonly APP_VERSION_KEY="CFBundleShortVersionString"
  readonly KS_VERSION_KEY="KSVersion"
  readonly KS_PRODUCT_KEY="KSProductID"
  readonly KS_CHANNEL_KEY="KSChannelID"
  readonly VERSIONS_DIR="Contents/Versions"
  readonly PRODUCT_URL="http://www.google.com/chrome/"
  readonly BUILD_RE="^[0-9]+\\.[0-9]+\\.([0-9]+)\\.[0-9]+\$"
  readonly MIN_BUILD=434

  local old_app_path="${old_fs}/${APP_NAME}"
  local old_app_plist="${old_app_path}/${APP_PLIST}"
  local old_app_version
  if ! old_app_version="$(defaults read "${old_app_plist}" \
                                        "${APP_VERSION_KEY}")"; then
    err "could not read old app version"
    exit 10
  fi
  if ! [[ "${old_app_version}" =~ ${BUILD_RE} ]]; then
    err "old app version not of expected format"
    exit 10
  fi
  local old_app_version_build="${BASH_REMATCH[1]}"

  local old_ks_plist="${old_app_plist}"
  local old_ks_version
  if ! old_ks_version="$(defaults read "${old_ks_plist}" \
                                       "${KS_VERSION_KEY}")"; then
    err "could not read old Keystone version"
    exit 10
  fi

  local new_app_path="${new_fs}/${APP_NAME}"
  local new_app_plist="${new_app_path}/${APP_PLIST}"
  local new_app_version
  if ! new_app_version="$(defaults read "${new_app_plist}" \
                      "${APP_VERSION_KEY}")"; then
    err "could not read new app version"
    exit 11
  fi
  if ! [[ "${new_app_version}" =~ ${BUILD_RE} ]]; then
    err "new app version not of expected format"
    exit 11
  fi
  local new_app_version_build="${BASH_REMATCH[1]}"

  local new_ks_plist="${new_app_plist}"
  local new_ks_version
  if ! new_ks_version="$(defaults read "${new_ks_plist}" \
                                       "${KS_VERSION_KEY}")"; then
    err "could not read new Keystone version"
    exit 11
  fi

  local new_ks_product
  if ! new_ks_product="$(defaults read "${new_app_plist}" \
                                       "${KS_PRODUCT_KEY}")"; then
    err "could not read new Keystone product ID"
    exit 11
  fi

  if [[ ${old_app_version_build} -lt ${MIN_BUILD} ]] ||
     [[ ${new_app_version_build} -lt ${MIN_BUILD} ]]; then
    err "old and new versions must be build ${MIN_BUILD} or newer"
    exit 12
  fi

  local new_ks_channel
  new_ks_channel="$(defaults read "${new_app_plist}" \
                    "${KS_CHANNEL_KEY}" 2> /dev/null || true)"

  local name_extra
  if [[ "${new_ks_channel}" = "beta" ]]; then
    name_extra=" Beta"
  elif [[ "${new_ks_channel}" = "dev" ]]; then
    name_extra=" Dev"
  elif [[ -n "${new_ks_channel}" ]]; then
    name_extra=" ${new_ks_channel}"
  fi

  local old_versioned_dir="${old_app_path}/${VERSIONS_DIR}/${old_app_version}"
  local new_versioned_dir="${new_app_path}/${VERSIONS_DIR}/${new_app_version}"

  if ! cp -p "${SCRIPT_DIR}/keystone_install.sh" \
             "${patch_fs}/.keystone_install"; then
    err "could not copy .keystone_install"
    exit 13
  fi

  local patch_dotpatch_dir="${patch_fs}/.patch"
  if ! mkdir "${patch_dotpatch_dir}"; then
    err "could not mkdir patch_dotpatch_dir"
    exit 13
  fi

  if ! cp -p "${SCRIPT_DIR}/dirpatcher.sh" \
             "${SCRIPT_DIR}/goobspatch" \
             "${patch_dotpatch_dir}/"; then
    err "could not copy dirpatcher.sh and goobspatch"
    exit 13
  fi

  if ! echo "${new_ks_product}" > "${patch_dotpatch_dir}/ks_product" ||
     ! echo "${old_app_version}" > "${patch_dotpatch_dir}/old_app_version" ||
     ! echo "${new_app_version}" > "${patch_dotpatch_dir}/new_app_version" ||
     ! echo "${old_ks_version}" > "${patch_dotpatch_dir}/old_ks_version" ||
     ! echo "${new_ks_version}" > "${patch_dotpatch_dir}/new_ks_version"; then
    err "could not write patch product or version information"
    exit 13
  fi
  local patch_ks_channel_file="${patch_dotpatch_dir}/ks_channel"
  if [[ -n "${new_ks_channel}" ]]; then
    if ! echo "${new_ks_channel}" > "${patch_ks_channel_file}"; then
      err "could not write Keystone channel information"
      exit 13
    fi
  else
    if ! touch "${patch_ks_channel_file}"; then
      err "could not write empty Keystone channel information"
      exit 13
    fi
  fi

  # The only visible contents of the disk image will be a README file that
  # explains the image's purpose.
  local new_app_version_extra="${new_app_version}${name_extra}"
  cat > "${patch_fs}/README.txt" << __EOF__ || \
      (err "could not write README.txt" && exit 13)
This disk image contains a differential updater that can update
${PRODUCT_NAME} from version ${old_app_version} to ${new_app_version_extra}.

This image is part of the auto-update system and is not independently
useful.

To install ${PRODUCT_NAME}, please visit <${PRODUCT_URL}>.
__EOF__

  local patch_versioned_dir="\
${patch_dotpatch_dir}/version_${old_app_version}_${new_app_version}.dirpatch"

  if ! "${DIRDIFFER}" "${old_versioned_dir}" \
                      "${new_versioned_dir}" \
                      "${patch_versioned_dir}"; then
    local status=${?}
    err "could not create a dirpatch for the versioned directory"
    exit $((${status} + 20))
  fi

  # Set DIRDIFFER_EXCLUDE to exclude the contents of the Versions directory,
  # but to include an empty Versions directory. The versioned directory was
  # already addressed in the preceding dirpatch.
  export DIRDIFFER_EXCLUDE="/${APP_NAME_RE}/Contents/Versions/"

  # Set DIRDIFFER_NO_DIFF to exclude files introduced by or modified by
  # Keystone channel and brand tagging and subsequent code signing.
  export DIRDIFFER_NO_DIFF="\
/${APP_NAME_RE}/Contents/\
(CodeResources|Info\\.plist|MacOS/${PRODUCT_NAME}|_CodeSignature/.*)$"

  local patch_app_dir="${patch_dotpatch_dir}/application.dirpatch"

  if ! "${DIRDIFFER}" "${old_app_path}" \
                      "${new_app_path}" \
                      "${patch_app_dir}"; then
    local status=${?}
    err "could not create a dirpatch for the application directory"
    exit $((${status} + 40))
  fi

  unset DIRDIFFER_EXCLUDE DIRDIFFER_NO_DIFF

  echo "${PRODUCT_NAME} ${old_app_version}-${new_app_version_extra} Update"
}

# package_patch_dmg creates a disk image at patch_dmg with the contents of
# patch_fs. The disk image's volume name is taken from volume_name. temp_dir
# is a work directory such as /tmp for the packager's use.
package_patch_dmg() {
  local patch_fs="${1}"
  local patch_dmg="${2}"
  local volume_name="${3}"
  local temp_dir="${4}"

  # Because most of the contents of ${patch_fs} are already compressed, the
  # overall compression on the disk image is mostly used to minimize the sizes
  # of the filesystem structures. In the presence of so much
  # already-compressed data, zlib performs better than bzip2, so use UDZO.
  if ! "${PKG_DMG}" \
           --verbosity 0 \
           --source "${patch_fs}" \
           --target "${patch_dmg}" \
           --tempdir "${temp_dir}" \
           --format UDZO \
           --volname "${volume_name}" \
           --config "openfolder_bless=0"; then
    err "disk image creation failed"
    exit 9
  fi
}

# make_patch_dmg mounts old_dmg and new_dmg, invokes make_patch_fs to prepare
# a patch filesystem, and then hands the patch filesystem to package_patch_dmg
# to create patch_dmg.
make_patch_dmg() {
  local old_dmg="${1}"
  local new_dmg="${2}"
  local patch_dmg="${3}"

  local temp_dir
  temp_dir="$(mktemp -d -t "${ME}")"
  g_cleanup+=("${temp_dir}")

  local old_mount_point="${temp_dir}/old"
  g_cleanup_mount_points+=("${old_mount_point}")
  if ! mount_dmg "${old_dmg}" "${old_mount_point}"; then
    err "could not mount old_dmg ${old_dmg}"
    exit 6
  fi

  local new_mount_point="${temp_dir}/new"
  g_cleanup_mount_points+=("${new_mount_point}")
  if ! mount_dmg "${new_dmg}" "${new_mount_point}"; then
    err "could not mount new_dmg ${new_dmg}"
    exit 7
  fi

  local patch_fs="${temp_dir}/patch"
  if ! mkdir "${patch_fs}"; then
    err "could not mkdir patch_fs ${patch_fs}"
    exit 8
  fi

  local volume_name
  volume_name="$(make_patch_fs "${old_mount_point}" \
                               "${new_mount_point}" \
                               "${patch_fs}")"

  hdiutil detach "${new_mount_point}" > /dev/null
  unset g_cleanup_mount_points[${#g_cleanup_mount_points[@]}]

  hdiutil detach "${old_mount_point}" > /dev/null
  unset g_cleanup_mount_points[${#g_cleanup_mount_points[@]}]

  package_patch_dmg "${patch_fs}" "${patch_dmg}" "${volume_name}" "${temp_dir}"

  rm -rf "${temp_dir}"
  unset g_cleanup[${#g_cleanup[@]}]
}

# shell_safe_path ensures that |path| is safe to pass to tools as a
# command-line argument. If the first character in |path| is "-", "./" is
# prepended to it. The possibily-modified |path| is output.
shell_safe_path() {
  local path="${1}"
  if [[ "${path:0:1}" = "-" ]]; then
    echo "./${path}"
  else
    echo "${path}"
  fi
}

usage() {
  echo "usage: ${ME} old_dmg new_dmg patch_dmg" >& 2
}

main() {
  local old_dmg new_dmg patch_dmg
  old_dmg="$(shell_safe_path "${1}")"
  new_dmg="$(shell_safe_path "${2}")"
  patch_dmg="$(shell_safe_path "${3}")"

  trap cleanup EXIT HUP INT QUIT TERM

  if ! [[ -f "${old_dmg}" ]] || ! [[ -f "${new_dmg}" ]]; then
    err "old_dmg and new_dmg must exist and be files"
    usage
    exit 3
  fi

  if [[ -e "${patch_dmg}" ]]; then
    err "patch_dmg must not exist"
    usage
    exit 4
  fi

  local patch_dmg_parent
  patch_dmg_parent="$(dirname "${patch_dmg}")"
  if ! [[ -d "${patch_dmg_parent}" ]]; then
    err "patch_dmg parent directory must exist and be a directory"
    usage
    exit 5
  fi

  make_patch_dmg "${old_dmg}" "${new_dmg}" "${patch_dmg}"

  trap - EXIT
}

if [[ ${#} -ne 3 ]]; then
  usage
  exit 2
fi

main "${@}"
exit ${?}
