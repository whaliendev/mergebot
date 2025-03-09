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

# Change ownership of REPOS_DIR to the appuser's UID
echo "Changing ownership of $REPOS_DIR to UID $USER_ID..."
if ! chown -R $USER_ID:$USER_ID "$REPOS_DIR"; then
  echo "Error: Failed to change ownership of $REPOS_DIR."
  exit 1
fi

chown -R $USER_ID /app

# create a new user with the same UID as the host user, named appuser
useradd --shell /bin/bash -u $USER_ID -o -c "" -m appuser
usermod -a -G root appuser
export HOME=/home/appuser

# su to appuser and run the application
exec /usr/sbin/gosu appuser bash -c "
  git config --global safe.directory '*' 
  git config --global user.name 'mergebot-backend'
  git config --global user.email 'mergebot-backend@gmail.com'
  git config --global merge.conflictstyle 'diff3'
  mvn spring-boot:run -Dmaven.repo.local=/app/.m2"

