package com.example.filemanager.services;

import com.example.filemanager.pojo.MergeScenario;
import com.example.filemanager.pojo.MergeTuple;
import org.eclipse.jgit.lib.ObjectLoader;
import org.eclipse.jgit.lib.Repository;
import org.eclipse.jgit.revwalk.RevCommit;
import org.eclipse.jgit.treewalk.TreeWalk;
import org.springframework.stereotype.Service;

import java.io.IOException;
import java.util.List;
@Service
public interface ConflictServices {

    public byte[] getFileByCommitAndPath(String path, RevCommit commit, Repository repository) throws IOException;

    public List<MergeScenario> getAllConflictInfo(String path) throws Exception;


    public List<MergeScenario> getMergeInfoByCommit(RevCommit merged,Repository repository)throws Exception;

    public List<MergeScenario> getSpecifiedConflictInfo(String path,String branch1,String branch2) throws Exception;

    public List<MergeScenario> getMergeInfo(RevCommit commit1,RevCommit commit2, Repository repository,String path) throws Exception;

    public int saveMergeInfo(String path,String branch1,String branch2) throws Exception;

    public MergeScenario getSpecifiedFile(String filePath,String repo,int isBinary) throws Exception;

    public List<MergeTuple> extractTuple(List<String> conflict,List<String> resolve,int isSolve) throws Exception;

    public int[] detectAutoMerge(MergeScenario mergeScenario);

    public List<String> getBranch(String path) throws Exception;

    public int checkFinish(String repo) throws Exception;
}
