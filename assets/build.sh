#!/bin/bash

# Define directory path
mergebot_dir=~/codebase/cpp/mergebot

# Check if mergebot directory exists
if [ ! -d "$mergebot_dir" ]; then
  echo "Error: mergebot directory not found at $mergebot_dir"
  exit 1
fi

# Remove the build directory
echo "Removing build directory..."
rm -rf "$mergebot_dir/build"

# Install dependencies and build Release version
echo "Installing dependencies and building Release version..."
(cd "$mergebot_dir" && conan install . --build=missing -r=conan -r=conancenter -s build_type=Release)
(cd "$mergebot_dir" && cmake --preset conan-release)
(cd "$mergebot_dir" && cmake --build build/Release)

# Install dependencies and build Debug version
echo "Installing dependencies and building Debug version..."
(cd "$mergebot_dir" && conan install . --build=missing -r=conan -r=conancenter -s build_type=Debug)
(cd "$mergebot_dir" && cmake --preset conan-debug)
(cd "$mergebot_dir" && cmake --build build/Debug)

echo "Operations completed successfully"
