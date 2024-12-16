# docker/backend/Dockerfile

# Use an official OpenJDK runtime as a parent image
FROM openjdk:17-jdk-slim

# Set working directory
WORKDIR /app

# Copy Maven or Gradle build files if applicable
# Assuming you're using Maven
COPY ui/conflict-manager/pom.xml ./
COPY ui/conflict-manager/src ./src

# install maven and gosu
RUN apt-get update && \
     apt-get install -y maven git gosu && \
     rm -rf /var/lib/apt/lists/* && \
     gosu nobody true && \
     mvn clean install -Dmaven.repo.local=/app/.m2

COPY docker/backend/docker-entrypoint.sh /usr/local/bin/docker-entrypoint.sh
RUN chmod a+x /usr/local/bin/docker-entrypoint.sh


# Expose the port the backend runs on
EXPOSE 8080

ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
