package com.example.filemanager.pojo;

import org.springframework.lang.NonNull;

public class ResolutionChoice {
    private Long id;
    private Long fileId; // foreign key to audit table
    private Integer blockIdx;
    private String choice;
    private Float similarity;
    private String similarTo;


    public ResolutionChoice(Long id, Long fileId, Integer blockIdx, String choice, Float similarity, String similarTo) {
        this.id = id;
        this.fileId = fileId;
        this.blockIdx = blockIdx;
        this.choice = choice;
        this.similarity = similarity;
        this.similarTo = similarTo;
    }

    public Long getId() {
        return id;
    }

    public void setId(@NonNull Long id) {
        this.id = id;
    }

    public Long getFileId() {
        return fileId;
    }

    public void setFileId(Long fileId) {
        this.fileId = fileId;
    }

    public Integer getBlockIdx() {
        return blockIdx;
    }

    public void setBlockIdx(Integer blockIdx) {
        this.blockIdx = blockIdx;
    }

    public String getChoice() {
        return choice;
    }

    public void setChoice(String choice) {
        this.choice = choice;
    }

    public Float getSimilarity() {
        return similarity;
    }

    public void setSimilarity(Float similarity) {
        this.similarity = similarity;
    }

    public String getSimilarTo() {
        return similarTo;
    }

    public void setSimilarTo(String similarTo) {
        this.similarTo = similarTo;
    }
}
