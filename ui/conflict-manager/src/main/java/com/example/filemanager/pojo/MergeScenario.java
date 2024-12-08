package com.example.filemanager.pojo;
import com.example.filemanager.utils.PathUtils;

import java.util.List;

public class MergeScenario {
    public String base;
    public String ours;
    public String theirs;
    public List<String> conflict;
    public List<String> originConflict;
    public List<String> apply;

    public String fileName;
    public String absPath;
    public String tempPath;

    public String targetPath;
    public String sourcePath;
    public String basePath;
    public String conflictPath;
    public MergeScenario(String filePath){
        this.absPath=filePath;
    }

}
