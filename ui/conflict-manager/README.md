## Conflict Manager
The backend service for the MergeBot platform.

This subproject is designed to manage metadata related to conflict merge scenarios in Git repositories. It leverages **jGit** for Git operations, supports extensive file handling, and records as well as audits recommended solutions along with their acceptance rates.

### Prerequisites
- **Java 8**
- **Maven**
- **MySQL**
- **Redis**

You can run MySQL and Redis locally or via Docker containers.

> **Note**: All of the following commands must be run from the conflict-manager directory.

### Development

#### 1. Set Up the Environment

1. **MySQL Setup**
    - Run a MySQL container (using Docker):
      ```bash
      docker run --name conflict-mysql -e MYSQL_ROOT_PASSWORD=123456 -e MYSQL_DATABASE=conflict -p 3306:3306 -d mysql:8.0
      ```
    - Wait for MySQL to initialize. Once running, you can import the initial schema:
      ```bash
      docker exec -i conflict-mysql mysql -uroot -p123456 conflict < conflict.sql
      ```

2. **Redis Setup**
    - Run a Redis container (using Docker):
      ```bash
      docker run --name conflict-redis -p 6379:6379 -d redis:latest
      ```

#### 2. Running the Application
- Once MySQL and Redis are running and the database schema is imported:
  ```bash
  mvn spring-boot:run
  ```
- The application will start on `http://localhost:8080` by default.

#### 3. Development Tips
- **Configuration Changes**: Most configuration is done via `src/main/resources/application.properties`.

- **Testing**:  
  Use:
  ```bash
  mvn test
  ```
  to run the tests. The project includes `spring-boot-starter-test` and `junit-jupiter-api` for testing purposes.

### Build

#### Steps to Build
1. **Clean and Package**:
   ```bash
   mvn clean package
   ```
   This will produce a JAR file in the `target` directory.

2. **Run the JAR**:
   After building, you can run:
   ```bash
   java -jar target/conflictmanager-2.0-SNAPSHOT.jar
   ```

3. **Production Deployment**:
   - Ensure that MySQL and Redis are properly set up in a production environment.
   - Adjust `spring.datasource.url`, `spring.datasource.username`, `spring.datasource.password`, `spring.redis.host`, and `spring.redis.port` as needed for your production environment.

