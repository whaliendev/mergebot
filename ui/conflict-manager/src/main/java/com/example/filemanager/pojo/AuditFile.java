package com.example.filemanager.pojo;

public class AuditFile {
    private Long id;
    private String projectPath;
    private String targetBranch;
    private String sourceBranch;
    private String fileName; // relative path to projectPath

    private Boolean liked;


    public AuditFile(Long id, String projectPath, String targetBranch, String sourceBranch, String fileName, Boolean liked) {
        this.id = id;
        this.projectPath = projectPath;
        this.targetBranch = targetBranch;
        this.sourceBranch = sourceBranch;
        this.fileName = fileName;
        this.liked = liked;
    }

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public String getProjectPath() {
        return projectPath;
    }

    public void setProjectPath(String projectPath) {
        this.projectPath = projectPath;
    }

    public String getTargetBranch() {
        return targetBranch;
    }

    public void setTargetBranch(String targetBranch) {
        this.targetBranch = targetBranch;
    }

    public String getSourceBranch() {
        return sourceBranch;
    }

    public void setSourceBranch(String sourceBranch) {
        this.sourceBranch = sourceBranch;
    }

    public String getFileName() {
        return fileName;
    }

    public void setFileName(String fileName) {
        this.fileName = fileName;
    }

    public Boolean getLiked() {
        return liked;
    }

    public void setLiked(Boolean liked) {
        this.liked = liked;
    }
}
