#!/bin/bash

USER_ID=${LOCAL_USER_ID:-9001}

# Check if REPOS_DIR is set and if it exists as a directory
if [ -z "$REPOS_DIR" ]; then
  echo "Error: REPOS_DIR is not set. Please set the REPOS_DIR environment variable."
  exit 1
fi

if [ ! -d "$REPOS_DIR" ]; then
  echo "Error: $REPOS_DIR is not a valid directory."
  exit 1
fi

chown -R $USER_ID /app

# create a new user with the same UID as the host user, named appuser
useradd --shell /bin/bash -u $USER_ID -o -c "" -m appuser
usermod -a -G root appuser
export HOME=/home/appuser

# export LD_LIBRARY_PATH and start mergebot
export LD_LIBRARY_PATH=/app/bin:/app/bin/dylib
exec /usr/local/bin/gosu appuser /app/bin/mergebot
