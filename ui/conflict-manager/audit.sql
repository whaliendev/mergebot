CREATE DATABASE /*!32312 IF NOT EXISTS */`conflict` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_bin */;

USE conflict;

DROP TABLE IF EXISTS audit_file;

CREATE TABLE audit_file
(
    id            BIGINT AUTO_INCREMENT PRIMARY KEY,
    project_path  VARCHAR(200) NOT NULL,
    target_branch CHAR(40) NOT NULL,
    source_branch CHAR(40) NOT NULL,
    file_name     VARCHAR(200) NOT NULL, -- relative path to projectPath
    liked         BOOLEAN,
    INDEX idx_project_target_source_file (project_path(200), target_branch, source_branch, file_name(200))
) ENGINE = InnoDB
  DEFAULT CHARSET = utf8mb4
  COLLATE = utf8mb4_bin;

DROP TABLE IF EXISTS resolution_choice;

CREATE TABLE resolution_choice
(
    id         BIGINT AUTO_INCREMENT PRIMARY KEY,
    file_id    BIGINT NOT NULL, -- foreign key to audit_file
    block_idx  INT    NOT NULL,
    choice     VARCHAR(20),
    similarity FLOAT,
    similar_to  VARCHAR(20),
    INDEX idx_fileId_blockIdx (file_id, block_idx)
) ENGINE = InnoDB
  DEFAULT CHARSET = utf8mb4
  COLLATE = utf8mb4_bin;

