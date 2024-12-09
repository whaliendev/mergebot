package com.example.filemanager.services.impl;

import com.example.filemanager.utils.ScoreUtils;
import com.example.filemanager.dao.SolveMapper;
import com.example.filemanager.dao.fileInfoMapper;
import com.example.filemanager.pojo.MergeScenario;
import com.example.filemanager.pojo.MergeTuple;
import com.example.filemanager.pojo.fileInfoWithBLOBs;
import com.example.filemanager.pojo.Solved;
import com.example.filemanager.services.ConflictServices;
import com.example.filemanager.utils.FileUtils;
import com.example.filemanager.utils.GitUtils;
import com.example.filemanager.utils.PathUtils;
import difflib.DiffUtils;
import difflib.Patch;
import org.eclipse.jgit.api.Git;
import org.eclipse.jgit.api.Status;
import org.eclipse.jgit.lib.ObjectId;
import org.eclipse.jgit.lib.ObjectLoader;
import org.eclipse.jgit.lib.Repository;
import org.eclipse.jgit.revwalk.RevCommit;
import org.eclipse.jgit.treewalk.TreeWalk;
import org.eclipse.jgit.merge.MergeStrategy;
import org.eclipse.jgit.merge.RecursiveMerger;
import org.eclipse.jgit.merge.ThreeWayMerger;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.io.*;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Set;

@Service
public class ConflictImpl implements ConflictServices {

    @Autowired
    fileInfoMapper fileInfoMapper;
    @Autowired
    SolveMapper solveMapper;

    static final String NO_BASE_STR = "NO_BASE_VERSION";

    private static final Logger logger = LoggerFactory.getLogger(ConflictImpl.class);

    @Override
    public byte[] getFileByCommitAndPath(String path, RevCommit commit, Repository repository) throws IOException {
        if (commit == null) {
            // 如果 commit 为 null，则直接返回 null，表示无法获取文件内容。 一个初始提交（root commit），这种情况通常发生在仓库中没有任何历史记录的情况下。
            // 以及可能存在于远程仓库和当前仓库无base的merge中
            return null;
        }
        TreeWalk treeWalk = TreeWalk.forPath(repository, path, commit.getTree());
        if (treeWalk == null)
            return null;
        ObjectLoader objectLoader = repository.open(treeWalk.getObjectId(0));
        return objectLoader.getBytes();
    }

    @Override
    public List<MergeScenario> getAllConflictInfo(String path) throws Exception {
        GitUtils gitUtils = new GitUtils();
        Repository repository = gitUtils.getRepository(path);
        List<RevCommit> mergeCommits = gitUtils.getMergeCommits(repository);
        List<MergeScenario> scenarios = new ArrayList<>();
        for (int i = 0; i < mergeCommits.size(); i++) {
            RevCommit commit = mergeCommits.get(i);
            System.out.println("analysing " + i);
            try {
                scenarios.addAll(getMergeInfoByCommit(commit, repository));
            } catch (Exception e) {
                e.printStackTrace();
                throw new RuntimeException(e);
            }
        }
        return scenarios;
    }

    @Override
    public List<MergeScenario> getMergeInfoByCommit(RevCommit merged, Repository repository) throws Exception {
        RevCommit p1 = merged.getParents()[0];
        RevCommit p2 = merged.getParents()[1];
        ThreeWayMerger merger = MergeStrategy.RECURSIVE.newMerger(repository, true);
        List<MergeScenario> scenarios = new ArrayList<>();

        if (!merger.merge(p1, p2)) {
            RecursiveMerger rMerger = (RecursiveMerger) merger;
            RevCommit base = (RevCommit) rMerger.getBaseCommitId();
            rMerger.getMergeResults().forEach((file, result) -> {
                if (file.endsWith(".java") && result.containsConflicts()) {
                    // logger.info("conflicts were found in {}", file);
                    MergeScenario scenario = new MergeScenario(file);
                    try {
                        // logger.info("collecting scenario in merged commit {}", merged.getName());
                        if (getFileByCommitAndPath(file, p1, repository) != null) {
                            scenario.ours = new String(getFileByCommitAndPath(file, p1, repository));
                        }
                        if (getFileByCommitAndPath(file, p2, repository) != null) {
                            scenario.theirs = new String(getFileByCommitAndPath(file, p2, repository));
                        }
                        if (getFileByCommitAndPath(file, base, repository) != null) {
                            scenario.base = new String(getFileByCommitAndPath(file, base, repository));
                        }
                        scenarios.add(scenario);
                    } catch (Exception e) {
                        throw new RuntimeException(e);
                    }
                }
            });
        }
        return scenarios;
    }

    @Override
    public List<MergeScenario> getSpecifiedConflictInfo(String path, String branch1, String branch2) throws Exception {
        GitUtils gitUtils = new GitUtils();
        Repository repository = gitUtils.getRepository(path);
        Git git = new Git(repository);
        ObjectId b1 = git.getRepository().resolve(branch1);
        ObjectId b2 = git.getRepository().resolve(branch2);
        RevCommit commit1 = gitUtils.getSpecificCommits(repository, b1);
        RevCommit commit2 = gitUtils.getSpecificCommits(repository, b2);

        List<MergeScenario> scenarios1 = getMergeInfo(commit1, commit2, repository, path);
        git.close();
        repository.close();
        return scenarios1;
    }

    @Override
    public List<MergeScenario> getMergeInfo(RevCommit commit1, RevCommit commit2, Repository repository, String path)
            throws Exception {
        ThreeWayMerger merger = MergeStrategy.RECURSIVE.newMerger(repository, true);
        List<MergeScenario> scenarios = new ArrayList<>();
        FileUtils fileUtils = new FileUtils();
        PathUtils pathUtils = new PathUtils();
        if (!merger.merge(commit1, commit2)) {
            RecursiveMerger rMerger = (RecursiveMerger) merger;
            RevCommit base = (RevCommit) rMerger.getBaseCommitId();
            rMerger.getMergeResults().forEach((file, result) -> {
                if (file.endsWith(".java") && result.containsConflicts()) {
                    MergeScenario scenario = new MergeScenario(file);
                    try {
                        if (getFileByCommitAndPath(file, commit1, repository) != null) {
                            scenario.ours = new String(getFileByCommitAndPath(file, commit1, repository));
                        }
                        if (getFileByCommitAndPath(file, commit2, repository) != null) {
                            scenario.theirs = new String(getFileByCommitAndPath(file, commit2, repository));
                        }
                        if (getFileByCommitAndPath(file, base, repository) != null) {
                            scenario.base = new String(getFileByCommitAndPath(file, base, repository));
                        }
                        scenario.absPath = pathUtils.getFileWithPathSegment(path, file);
                        File conflict = new File(scenario.absPath);
                        scenario.conflict = fileUtils.readFile(conflict);
                        scenarios.add(scenario);
                    } catch (Exception e) {
                        throw new RuntimeException(e);
                    }
                }
            });
        }
        return scenarios;
    }

    @Override
    public int saveMergeInfo(String path, String branch1, String branch2) throws Exception {
        GitUtils gitUtils = new GitUtils();
        Repository repository = gitUtils.getRepository(path);
        try (Git git = new Git(repository)) {
            // ObjectId b1 = git.getRepository().resolve(branch1);
            // ObjectId b2 = git.getRepository().resolve(branch2);
            ProcessBuilder processBuilder = new ProcessBuilder("git", "rev-parse", branch1 + "^{commit}");
            processBuilder.directory(new File(path));
            Process process = processBuilder.start();
            BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
            String line = reader.readLine();
            ObjectId b1 = ObjectId.fromString(line);
            // 寻找提交节点
            processBuilder = new ProcessBuilder("git", "rev-parse", branch2 + "^{commit}");
            processBuilder.directory(new File(path));
            process = processBuilder.start();
            reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
            line = reader.readLine();
            ObjectId b2 = ObjectId.fromString(line);
            RevCommit commit1 = gitUtils.getSpecificCommits(repository, b1);
            RevCommit commit2 = gitUtils.getSpecificCommits(repository, b2);
            ThreeWayMerger merger = MergeStrategy.RECURSIVE.newMerger(repository, true);
            PathUtils pathUtils = new PathUtils();
            Status status = git.status().call();
            Set<String> conflictSet = status.getConflicting();
            List<fileInfoWithBLOBs> info = fileInfoMapper.getAllFiles(path);
            // 判断是否存在合并冲突
            if (!conflictSet.isEmpty() && info.isEmpty()) {
                if (commit2 != null) {
                    merger.merge(commit1, commit2);
                    logger.info("commit {} exists-----", commit2.getName());
                } else {
                    logger.info("commit {} does not exists-----", branch2);
                }
                logger.info("the number of conflicts are {}-----", conflictSet.size());
                RecursiveMerger rMerger = (RecursiveMerger) merger;
                for (String file : conflictSet) {
                    List<String> filepath = Arrays.asList(file.split("/"));
                    String fileName = filepath.get(filepath.size() - 1);
                    fileInfoWithBLOBs fileInfo = new fileInfoWithBLOBs(fileName, 1);
                    try {
                        fileInfo.setOurs(b1.getName());
                        if (commit2 != null) {
                            fileInfo.setTheirs(b2.getName());
                            // 2024-02-26(hwa) 修复冲突时，baseCommitId为空的问题
                            fileInfo.setBase(rMerger.getBaseCommitId() == null ? NO_BASE_STR
                                    : rMerger.getBaseCommitId().getName());
                        }
                    } catch (Exception e) {
                        throw new RuntimeException(e);
                    }
                    fileInfo.setPath(pathUtils.getFileWithPathSegment(path, file));
                    fileInfo.setRelPath(file);
                    fileInfo.setRepo(path);
                    fileInfoMapper.insert(fileInfo);
                    logger.info("collect conflict file {}-----", file);

                    // 2024-05-07(hwa) 修改判断二进制文件的方法以提高效率
                    // 原方法： 读取整个文件，通过 tika 判断 mime type
                    // 现方法： 读取前 8k 字符，首先判断BOM，然后判断 printable 和 nonprintable 相对数量
                    // Benchmark                              Mode  Cnt    Score     Error  Units
                    // IsBinaryBench.benchmarkIsBinaryJGit    avgt    5  290.420 ±  14.433  ms/op
                    // IsBinaryBench.benchmarkIsBinaryManual  avgt    5   19.354 ±   2.149  ms/op
                    // IsBinaryBench.benchmarkIsBinaryTika    avgt    5  886.556 ± 143.510  ms/op
                    // 实测效率提升 30 倍 ～ 40 倍，准确率几乎 100%
                    try {
                        if (!FileUtils.isBinaryFile2(fileInfo.getPath())) {
                            git.add().addFilepattern(fileInfo.getRelPath()).call();
                        }
                    } catch (IllegalArgumentException | IOException e) {
                        logger.error("failed to stage file {}, reason: {}", file, e.getMessage());
                    }
                }
                // git.add().addFilepattern(".").call();
            }
            git.close();
            repository.close();
        }
        return 1;

    }

    @Override
    public MergeScenario getSpecifiedFile(String filePath, String repo, int isBinary) throws Exception {
        GitUtils gitUtils = new GitUtils();
        Repository repository = gitUtils.getRepository(repo);
        try (Git git = new Git(repository)) {
            MergeScenario mergeScenario = new MergeScenario(filePath);
            FileUtils fileUtils = new FileUtils();
            PathUtils pathUtils = new PathUtils();
            fileInfoWithBLOBs fileInfo = fileInfoMapper.selectByPrimaryKey(pathUtils.getSystemCompatiblePath(filePath));
            byte[] binaryOurs = null;
            byte[] binaryTheirs = null;
            byte[] binaryBase = null;
            // 如果不是二进制文件的话
            // 根据数据库中的相对路径获取文件内容
            if (isBinary == 0 && fileInfo != null) {
                // logger.info("the abs path of the file is {}-----",fileInfo.getPath());
                // logger.info("the rel path of the file is {}-----",fileInfo.getRelPath());
                if (fileInfo.getOurs() != null) {
                    ObjectId oursId = git.getRepository().resolve(fileInfo.getOurs());
                    RevCommit oursCommit = gitUtils.getSpecificCommits(repository, oursId);
                    binaryOurs = getFileByCommitAndPath(fileInfo.getRelPath(), oursCommit, repository);
                    if (binaryOurs != null) {
                        mergeScenario.ours = new String(binaryOurs);
                    }
                }

                if (fileInfo.getTheirs() != null) {
                    ObjectId theirsId = git.getRepository().resolve(fileInfo.getTheirs());
                    RevCommit theirsCommit = gitUtils.getSpecificCommits(repository, theirsId);
                    binaryTheirs = getFileByCommitAndPath(fileInfo.getRelPath(), theirsCommit, repository);
                    if (binaryTheirs != null) {
                        mergeScenario.theirs = new String(binaryTheirs);
                    }
                }
                if (fileInfo.getBase() != null) {
                    ObjectId baseId = git.getRepository().resolve(fileInfo.getBase());
                    RevCommit baseCommit = gitUtils.getSpecificCommits(repository, baseId);
                    binaryBase = getFileByCommitAndPath(fileInfo.getRelPath(), baseCommit, repository);
                    if (binaryBase != null) {
                        mergeScenario.base = new String(binaryBase);
                    }
                }
                if (fileInfo.getPath() != null) {
                    mergeScenario.apply = fileUtils.readFile(new File(fileInfo.getPath()));
                }

                // 需要修改
                // File conflict=new File(fileInfo.getPath());
                Path tmpNoPrefix = Files.createTempDirectory(null);
                String path = tmpNoPrefix.toString();

                fileUtils.write(pathUtils.getFileWithPathSegment(path, "ours.java"), binaryOurs);
                fileUtils.write(pathUtils.getFileWithPathSegment(path, "theirs.java"), binaryTheirs);
                fileUtils.write(pathUtils.getFileWithPathSegment(path, "base.java"), binaryBase);
                fileUtils.write(pathUtils.getFileWithPathSegment(path, "conflict.java"), binaryOurs);
                File ours = new File(path, "ours.java");
                File theirs = new File(path, "theirs.java");
                File base = new File(path, "base.java");
                File conflict = new File(path, "conflict.java");
                ProcessBuilder pb2 = new ProcessBuilder(
                        "git",
                        "merge-file",
                        "--diff3",
                        conflict.getPath(),
                        base.getPath(),
                        theirs.getPath());
                try {
                    pb2.start().waitFor();
                    pb2.start().destroy();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                mergeScenario.conflict = fileUtils.readFile(new File(fileInfo.getPath()));
                mergeScenario.absPath = fileInfo.getPath();
                mergeScenario.tempPath = path;
                mergeScenario.targetPath = ours.getPath();
                mergeScenario.sourcePath = theirs.getPath();
                mergeScenario.basePath = base.getPath();
                mergeScenario.conflictPath = fileInfo.getPath();
            }
            // 删除临时文件
            // org.apache.commons.io.FileUtils.deleteDirectory(new File(path));
            repository.close();
            git.close();
            return mergeScenario;
        }
    }

    /*
     * 在打开新冲突文件时，提取冲突文件中的冲突块，并查找相似的解决方案
     * 在解决冲突后写入文件，提取解决方案和冲突块，存入solve表中待下次查取
     */
    @Override
    public List<MergeTuple> extractTuple(List<String> conflict, List<String> resolve, int isSolve) throws Exception {
        List<String> copy = new ArrayList<>();
        List<MergeTuple> tupleList = new ArrayList<>();
        FileUtils fileUtils = new FileUtils();
        ScoreUtils scoreUtils = new ScoreUtils();
        try {
            for (int i = 0, cnt = 0; i < conflict.size(); ++i) {
                // 如果不是冲突块，则将这一行加入copy？然后继续，cnt++
                if (!conflict.get(i).startsWith("<<<<<<< ")) {
                    copy.add(conflict.get(i));
                    cnt++;
                    continue;
                }
                // 如果是冲突块，则生成一个冲突块，mark为起始的行数（原文件）
                MergeTuple tuple = new MergeTuple();
                tuple.mark = cnt;
                // j，k分别为标记？
                int j = i, k = i, isBinary = 0;
                tuple.start = j + 1;
                try {
                    while (!conflict.get(k).startsWith("||||||| ")) {
                        k++;
                        if (k >= conflict.size()) {
                            isBinary = 1;
                            k = i;
                            break;
                        }
                    }
                    // j此时为下一个冲突，也就是base中冲突开始的位置
                    tuple.middle = k + 1;
                    while (!conflict.get(k).equals("=======")) {
                        k++;
                    }
                    tuple.middle2 = k + 1;
                    if (isBinary == 1) {
                        tuple.middle = k + 1;
                    }
                    while (!conflict.get(k).startsWith(">>>>>>> ")) {
                        k++;
                    }
                    tuple.end = k + 1;
                    tuple.ours = fileUtils.getCodeSnippets(conflict, tuple.start - 1, tuple.middle - 1);
                    tuple.base = fileUtils.getCodeSnippets(conflict, tuple.middle - 1, tuple.middle2 - 1);
                    tuple.theirs = fileUtils.getCodeSnippets(conflict, tuple.middle2 - 1, tuple.end - 1);
                    // logger.info("start {} middle1 {} middle2{} end
                    // {}-----",tuple.start,tuple.middle,tuple.middle2,tuple.end);
                } catch (IndexOutOfBoundsException e) {
                    e.printStackTrace();
                }
                // 从k继续遍历
                i = k;
                tupleList.add(tuple);
            }
        } catch (IndexOutOfBoundsException e) {
            e.printStackTrace();
        }
        if (isSolve == 1) {
            int rec[] = fileUtils.alignLines(copy, resolve);
            for (MergeTuple tuple : tupleList) {
                int mark = tuple.mark;

                if (mark > 0 && mark < copy.size() && rec[mark - 1] != -1 && rec[mark] != -1) {
                    tuple.resolve = resolve.subList(rec[mark - 1] + 1, rec[mark]);
                }
            }
        }
        if (isSolve == 0) {
            for (MergeTuple tuple : tupleList) {
                StringBuilder c = new StringBuilder();
                for (String line : tuple.ours) {
                    c.append(line).append("\n");
                }
                for (String line : tuple.base) {
                    c.append(line).append("\n");
                }
                for (String line : tuple.theirs) {
                    c.append(line).append("\n");
                }
                List<Solved> truths = solveMapper.searchResolved(c.toString());
                for (Solved truth : truths) {
                    tuple.historyTruth.add(truth.solve);
                }
                int n = 3;
                double[] scores = new double[n];
                for (int i = 0; i < (Math.min(n, truths.size())); i++) {
                    scores[i] = scoreUtils.checkSimilarity(truths.get(i).chunk, c.toString());
                }
                tuple.truthScore = scores;
            }
        }
        return tupleList;
    }

    @Override
    public int[] detectAutoMerge(MergeScenario mergeScenario) {
        int[] rec = new int[mergeScenario.conflict.size()];
        List<String> ours = Arrays.asList(mergeScenario.ours.split("\n"));
        Patch<String> patch = DiffUtils.diff(ours, mergeScenario.conflict);
        List<String> diff = DiffUtils.generateUnifiedDiff("", "", ours, patch, mergeScenario.conflict.size());
        for (int i = 0, k = 3; k < diff.size(); ++k) {
            char c = diff.get(k).charAt(0);
            if (c == '+') {
                rec[i] = 1;
                i++;
            } else if (c != '-' && c != '+') {
                i++;
            }
        }
        return rec;
    }

    @Override
    public List<String> getBranch(String path) throws Exception {
        // GitUtils gitUtils = new GitUtils();
        // Repository repository = gitUtils.getRepository(path);
        // try (Git git = new Git(repository)) {
        // List<Ref> call = git.branchList().call();
        ProcessBuilder processBuilder = new ProcessBuilder("git", "branch");
        processBuilder.directory(new File(path));
        logger.info("run git branch in direcotry {}", path);
        try {
            // 执行Git命令
            Process process = processBuilder.start();
            // 读取命令行输出
            InputStream inputStream = process.getInputStream();
            BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
            String line;
            List<String> branches = new ArrayList<>();
            while ((line = reader.readLine()) != null) {
                branches.add(line.substring(2));
            }
            processBuilder = new ProcessBuilder("git", "tag");
            processBuilder.directory(new File(path));
            Process process2 = processBuilder.start();
            inputStream = process2.getInputStream();
            reader = new BufferedReader(new InputStreamReader(inputStream));
            while ((line = reader.readLine()) != null) {
                branches.add(line);
            }
            // for (Ref ref : call) {
            // branches.add(ref.getName());
            // }
            // repository.close();
            // git.close();
            return branches;
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public int checkFinish(String repo) throws Exception {
        List<fileInfoWithBLOBs> files = fileInfoMapper.getUnsolved(repo);
        return files.size();
    }
}
