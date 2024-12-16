package space.whalien.conflictmanager.pojo.vo;

public class BlockResolutionChoiceRequest {
    private String projectPath;
    private String targetBranch;
    private String sourceBranch;
    private String fileName;
    private Integer blockIdx;
    private BlockResolutionChoiceVO blockResolutionChoice;

    public BlockResolutionChoiceRequest() {}

    public BlockResolutionChoiceRequest(String projectPath, String targetBranch, String sourceBranch, String fileName, Integer blockIdx, BlockResolutionChoiceVO blockResolutionChoice) {
        this.projectPath = projectPath;
        this.targetBranch = targetBranch;
        this.sourceBranch = sourceBranch;
        this.fileName = fileName;
        this.blockIdx = blockIdx;
        this.blockResolutionChoice = blockResolutionChoice;
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

    public Integer getBlockIdx() {
        return blockIdx;
    }

    public void setBlockIdx(Integer blockIdx) {
        this.blockIdx = blockIdx;
    }

    public BlockResolutionChoiceVO getBlockResolutionChoice() {
        return blockResolutionChoice;
    }

    public void setBlockResolutionChoice(BlockResolutionChoiceVO blockResolutionChoice) {
        this.blockResolutionChoice = blockResolutionChoice;
    }
}
