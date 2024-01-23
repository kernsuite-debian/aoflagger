#!/bin/bash

# Copyright (C) 2021 ASTRON (Netherlands Institute for Radio Astronomy)
# SPDX-License-Identifier: GPL-3.0-or-later

#Script configuration for this repo. Adjust it when copying to a different repo.

#The directory that contains the source files.
SOURCE_DIR=$(dirname "$0")/..

#Directories that must be excluded from formatting. These paths are
#relative to SOURCE_DIR.
EXCLUDE_DIRS=(external build CMake)

# Use a specific clang-format version, since formatting differs between versions.
CLANG_FORMAT_BINARY=clang-format-14

#End script configuration.

#The common formatting script has further documentation.
source $(dirname "$0")/../external/aocommon/scripts/format.sh

# Run the Lua formatter.
if ! which stylua &>/dev/null; then
  echo Warning: stylua not found, not running Lua formatting.
else
  echo Running StyLua...
  STYLUA_ARGS="--indent-type spaces --indent-width 2 ${SOURCE_DIR}/data/strategies"
  if [ -n "$DRYRUN" ]; then
    if ! stylua --check $STYLUA_ARGS; then
      # Print in bold-face red
      echo -e "\e[1m\e[31mAt least one LUA file is not properly formatted!\e[0m"
      echo -e "\e[1m\e[31mRun $0 for formatting all files!\e[0m"
      exit 1
    else
      # print in bold-face green
      echo -e "\e[1m\e[32mGreat job, all LUA files are properly formatted!\e[0m"
    fi
  else
    stylua $STYLUA_ARGS
  fi
fi
