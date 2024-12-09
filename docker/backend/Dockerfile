# docker/backend/Dockerfile

# Use an official OpenJDK runtime as a parent image
FROM openjdk:17-jdk-slim

# Set working directory
WORKDIR /app

# Copy Maven or Gradle build files if applicable
# Assuming you're using Maven
COPY ui/conflict-manager/pom.xml ./
COPY ui/conflict-manager/src ./src

# If using Maven, install dependencies and build
RUN apt-get update && \
     apt-get install -y maven git && \
     rm -rf /var/lib/apt/lists/* && \
     git config --global safe.directory '*' && \
     git config --global user.name "mergebot-backend" && \
     git config --global user.email "mergebot-backend@gmail.com" && \
     mvn clean install


# Expose the port the backend runs on
EXPOSE 8080

# Define the command to run the backend
# Adjust the jar file name as necessary
CMD ["mvn", "spring-boot:run"]
