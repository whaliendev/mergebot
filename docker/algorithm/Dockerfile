# docker/algorithm/Dockerfile

# Use a lightweight base image suitable for running your pre-built binaries
FROM ubuntu:24.04

# Set the working directory inside the container
WORKDIR /app

# install gosu for further change user in entrypoint.sh
ENV GOSU_VERSION 1.17
RUN set -eux; \
# save list of currently installed packages for later so we can clean up
	savedAptMark="$(apt-mark showmanual)"; \
	apt-get update; \
	apt-get install -y --no-install-recommends ca-certificates gnupg wget; \
	rm -rf /var/lib/apt/lists/*; \
	\
	dpkgArch="$(dpkg --print-architecture | awk -F- '{ print $NF }')"; \
	wget -O /usr/local/bin/gosu "https://github.com/tianon/gosu/releases/download/$GOSU_VERSION/gosu-$dpkgArch"; \
	wget -O /usr/local/bin/gosu.asc "https://github.com/tianon/gosu/releases/download/$GOSU_VERSION/gosu-$dpkgArch.asc"; \
	\
# verify the signature
	export GNUPGHOME="$(mktemp -d)"; \
	gpg --batch --keyserver hkps://keys.openpgp.org --recv-keys B42F6819007F00F88E364FD4036A9C25BF357DD4; \
	gpg --batch --verify /usr/local/bin/gosu.asc /usr/local/bin/gosu; \
	gpgconf --kill all; \
	rm -rf "$GNUPGHOME" /usr/local/bin/gosu.asc; \
	\
# clean up fetch dependencies
	apt-mark auto '.*' > /dev/null; \
	[ -z "$savedAptMark" ] || apt-mark manual $savedAptMark; \
	apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false; \
	\
	chmod +x /usr/local/bin/gosu; \
# verify that the binary works
	gosu --version; \
	gosu nobody true

# install git for internal use
RUN apt-get update && apt-get install -y git && \
    rm -rf /var/lib/apt/lists/* && \
    git config --global safe.directory '*' && \
    git config --global user.name "mergebot-sa" && \
    git config --global user.email "mergebot-sa@gmail.com"

COPY docker/algorithm/docker-entrypoint.sh /usr/local/bin/docker-entrypoint.sh
RUN chmod a+x /usr/local/bin/docker-entrypoint.sh

# Copy the pre-built binaries from the host to the container
# This assumes that the build context includes the 'build/Release/bin' directory
COPY build/Release/bin /app/bin

# Ensure that all binaries have execute permissions
RUN chmod +x /app/bin/*

# Expose the port that the algorithm module listens on (adjust if different)
EXPOSE 18080

ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
