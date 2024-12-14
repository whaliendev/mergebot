package space.whalien.conflictmanager.services;

public interface GitService {
    public void mergeBranch(String path,String b1,String b2) throws Exception;

    public String commitAll(String path,String message) throws Exception;

    void reset(String repoPath,String fileName) throws Exception;

    public String commitAllGerrit(String path,String message, String target, String topic) throws Exception;

}
