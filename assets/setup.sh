#!/bin/bash

# fail fast
set -euo pipefail

function find_mergebot_binary() {
  local mergebot_dir=""
  if [ -x "./mergebot" ] || [ -x "$(command -v mergebot)" ]; then
    mergebot_dir="$(dirname "$(command -v mergebot)")"
    echo "-- mergebot binary found in current dir: $mergebot_dir" >&2
  else
    if [ -d "src" ]; then
      if [ -d "./build/Debug/bin" ] && [ -d "./build/Release/bin" ]; then
        if [ "./build/Debug/bin" -nt "./build/Release/bin" ]; then
          mergebot_dir="$(realpath "./build/Debug/bin")"
        else
          mergebot_dir="$(realpath "./build/Release/bin")"
        fi
        echo "-- newer build configuration chose: $mergebot_dir" >&2
      elif [ -d "./build/Debug" ] || [ -d "./build/Release" ]; then
        if [ -d "./build/Debug" ]; then
          echo "-- mergebot binary found in debug configuration" >&2
          mergebot_dir="$(realpath "./build/Debug/bin")"
        else
          echo "-- mergebot binary found in release configuration" >&2
          mergebot_dir="$(realpath "./build/Release/bin")"
        fi
      else
        echo "Error: You should run \`cmake\` to generate configuration first" >&2
        exit 1
      fi
    else
      echo "Error: this script must be run in mergebot's root directory" >&2
      exit 1
    fi
  fi
  echo "$mergebot_dir"
}

if [ $# -gt 0 ] && [ "$1" == "--dir-only" ]; then
  MB_BIN_DIR=$(find_mergebot_binary 2> /dev/null)
  echo "$MB_BIN_DIR"
  exit 0
fi

# Step 1: Find mergebot binary and set MB_BIN_DIR
MB_BIN_DIR=$(find_mergebot_binary)

# Step 3: Extract and copy .conan2 libraries to MB_BIN_DIR/dylib
if [ ! -d "$MB_BIN_DIR/dylib" ]; then
  echo "-- creating $MB_BIN_DIR/dylib..."
  mkdir "$MB_BIN_DIR/dylib"
else
  echo "-- $MB_BIN_DIR/dylib already exists, skipping creation..."
fi

ldd_output=$(ldd "$MB_BIN_DIR/mergebot")

while IFS= read -r line; do
  if [[ "$line" == *".conan2"* ]]; then
    lib_path=$(echo "$line" | awk '{print $3}')
    lib_name=$(basename "$lib_path")
    if [ ! -f "$MB_BIN_DIR/dylib/$lib_name" ]; then
      echo "-- copying: $lib_path to {MB_BIN_DIR}/dylib/$lib_name..."
      cp "$lib_path" "$MB_BIN_DIR/dylib/$lib_name"
    else
      echo "-- $lib_name already exists in $MB_BIN_DIR/dylib, skipping copy..."
    fi
  fi
done <<< "$ldd_output"

echo "-- setup completed"
