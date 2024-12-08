package com.example.filemanager.services;

import com.example.filemanager.pojo.fileInfoWithBLOBs;
import com.example.filemanager.pojo.vo.FileTree;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.List;

public interface fileServices {
    public List<Object> getAllDirection(String filePath);


    public  List<Object> getAll(String directoryPath);


    public  List<FileTree> getAllFiles(String directoryPath, String repoPath, List<fileInfoWithBLOBs> conflictFiles, List<fileInfoWithBLOBs> allFiles);

    public void write2ConflictFile(String content,String path,String fileName,String repo,String tempPath);

    List<FileTree> filterFiles(String path);

    int chooseBinaryFile(String path,String fileName,int branch) throws Exception;

    List<String> getFileContent(String filePath) throws Exception;
    void renameFileAndUpdateTree(String filePath, String newFileName,String repoPath) throws FileNotFoundException;
    void addFileAndUpdateTree(String filePath,String fileType) throws IOException;
    void deleteFileAndUpdateTree(String filePath,String repoPath) throws IOException;

}

