package space.whalien.conflictmanager.services;

import space.whalien.conflictmanager.pojo.FileInfoWithBlobs;
import space.whalien.conflictmanager.pojo.vo.FileTree;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.List;

public interface FileService {
    public List<Object> getAllDirection(String filePath);


    public  List<Object> getAll(String directoryPath);


    public  List<FileTree> getAllFiles(String directoryPath, String repoPath, List<FileInfoWithBlobs> conflictFiles, List<FileInfoWithBlobs> allFiles);

    public void write2ConflictFile(String content,String path,String fileName,String repo,String tempPath);

    List<FileTree> filterFiles(String path);

    int chooseBinaryFile(String path,String fileName,int branch) throws Exception;

    List<String> getFileContent(String filePath) throws Exception;
    void renameFileAndUpdateTree(String filePath, String newFileName,String repoPath) throws FileNotFoundException;
    void addFileAndUpdateTree(String filePath,String fileType) throws IOException;
    void deleteFileAndUpdateTree(String filePath,String repoPath) throws IOException;

}

