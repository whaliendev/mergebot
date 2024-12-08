package com.example.filemanager.pojo;

public class Solved {
    public String chunk;
    public String solve;

    public String path;
    public Solved(String chunk, String solve,String path) {
        this.chunk = chunk;
        this.solve = solve;
        this.path=path;
    }

    public String getChunk() {
        return chunk;
    }

    public String getSolve() {
        return solve;
    }

    public void setChunk(String chunk) {
        this.chunk = chunk;
    }

    public void setSolve(String solve) {
        this.solve = solve;
    }

}
