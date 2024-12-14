package space.whalien.conflictmanager.pojo.vo;

import java.util.List;

public class FileTree {
    public String fileName;
    public String filePath;
    public String path;
    public String fileType;
    public int conflictNumber;
    public int isConflictFile;
    public int isBinary;

    public List<FileTree> childTree;



    public FileTree(String fileName, String filePath,String path, String fileType, int conflictNumber, int isConflictFile, List<FileTree> childTree,int isBinary) {
        this.fileName = fileName;
        this.filePath = filePath;
        this.fileType = fileType;
        this.conflictNumber = conflictNumber;
        this.isConflictFile = isConflictFile;
        this.childTree = childTree;
        this.path=path;
        this.isBinary=isBinary;
    }

    public FileTree() {

    }
}
