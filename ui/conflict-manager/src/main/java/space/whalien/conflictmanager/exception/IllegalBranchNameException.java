package space.whalien.conflictmanager.exception;

public class IllegalBranchNameException extends FileManagerBaseException{
    private static final String message = "分支名非法";
    public IllegalBranchNameException(String projectPath, String branchName) {
        super(String.format("%s: 仓库路径 %s, 分支名 %s", message, projectPath, branchName));
    }
}
