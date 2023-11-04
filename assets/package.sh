#!/bin/bash

# Check for arguments
if [ $# -lt 1 ]; then
  echo "Usage: $0 VERSION [RELEASE]"
  exit 1
fi

VERSION="$1"
RELEASE="${2:-Ubuntu 16.04-x64}"

# Define directory paths
debug_dir=~/codebase/cpp/mergebot/build/Debug/bin
release_dir=~/codebase/cpp/mergebot/build/Release/bin
out_dir=~/Documents/mergebot-${VERSION}

# Get the parent directory of out_dir
out_parent=$(dirname "$out_dir")

# Check if Debug and Release directories exist
if [ ! -d "$debug_dir" ]; then
  echo "Error: Debug directory not found at $debug_dir"
  exit 1
fi

if [ ! -d "$release_dir" ]; then
  echo "Error: Release directory not found at $release_dir"
  exit 1
fi

# Check if setup.sh has execute permission
if [ ! -x "$debug_dir/setup.sh" ]; then
  echo "Error: setup.sh in Debug directory does not have execute permission"
  exit 1
fi

if [ ! -x "$release_dir/setup.sh" ]; then
  echo "Error: setup.sh in Release directory does not have execute permission"
  exit 1
fi

# Create output directory if it doesn't exist
if [ ! -d "$out_dir" ]; then
  mkdir -p "$out_dir"
  echo "Created directory: $out_dir"
fi

# Run setup.sh in Debug directory
(cd "$debug_dir" && ./setup.sh)

# Copy Debug directory to the output directory
cp -r "$debug_dir" "$out_dir/Debug"

# Create a zip file for the Debug directory
(cd "$out_dir" && zip -r "mergebot-sa_${VERSION}_Debug_${RELEASE}.zip" "Debug")
echo "Created Debug zip file: mergebot-sa_${VERSION}_Debug_${RELEASE}.zip"

# Delete the Debug directory
rm -r "$out_dir/Debug"
echo "Deleted Debug directory"

# Run setup.sh in Release directory
(cd "$release_dir" && ./setup.sh)

# Copy Release directory to the output directory
cp -r "$release_dir" "$out_dir/Release"

# Create a zip file for the Release directory
(cd "$out_dir" && zip -r "mergebot-sa_${VERSION}_Release_${RELEASE}.zip" "Release")
echo "Created Release zip file: mergebot-sa_${VERSION}_Release_${RELEASE}.zip"

# Delete the Release directory
rm -r "$out_dir/Release"
echo "Deleted Release directory"

# Create the final zip file for the output directory in out_parent
(cd "$out_parent" && zip -rj "mergebot-sa_${VERSION}_${RELEASE}.zip" "$out_dir")
echo "Created final zip file: mergebot-sa_${VERSION}_${RELEASE}.zip"

# Delete all files in out_dir
rm -r "$out_dir"
echo "Deleted all files in $out_dir"

echo "Operation completed successfully"
