# docker/algorithm/Dockerfile

# Use a lightweight base image suitable for running your pre-built binaries
FROM ubuntu:24.04

# Set the working directory inside the container
WORKDIR /app

# install git for internal use
RUN apt-get update && apt-get install -y git && \
    rm -rf /var/lib/apt/lists/* && \
    git config --global safe.directory '*' && \
    git config --global user.name "mergebot-backend" && \
    git config --global user.email "mergebot-backend@gmail.com"

# Copy the pre-built binaries from the host to the container
# This assumes that the build context includes the 'build/Release/bin' directory
COPY build/Release/bin /app/bin

# Ensure that all binaries have execute permissions
RUN chmod +x /app/bin/*

# Expose the port that the algorithm module listens on (adjust if different)
EXPOSE 18080

# Define the default command to run your main executable
CMD LD_LIBRARY_PATH=/app/bin:/app/bin/dylib /app/bin/mergebot
