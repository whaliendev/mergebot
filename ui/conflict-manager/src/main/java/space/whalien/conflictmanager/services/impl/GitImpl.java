package com.example.filemanager.services.impl;

import com.example.filemanager.dao.FileInfoMapper;
import com.example.filemanager.pojo.FileInfoWithBlobs;
import com.example.filemanager.services.GitService;
import com.example.filemanager.utils.GitUtils;
import com.example.filemanager.utils.PathUtils;

import org.eclipse.jgit.api.Git;
import org.eclipse.jgit.lib.Repository;
import org.eclipse.jgit.lib.RepositoryState;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;


@Service
public class GitImpl implements GitService {
    @Autowired
    FileInfoMapper fileInfoMapper;
    private static final Logger logger = LoggerFactory.getLogger(GitImpl.class);
    @Override
    public void mergeBranch(String path, String b1, String b2) throws Exception {
        GitUtils gitUtils=new GitUtils();
        Repository repository=gitUtils.getRepository(path);
        // 判断是否处于合并状态
        RepositoryState repositoryState = repository.getRepositoryState();
//        System.out.println("repositoryState mergeBranch 0:"+repositoryState);
        // 检查是否处于合并状态   24-4：merge保证处于冲突中。abort 应该处于另外的方法中（目前可以单独文件回退）
        if (repositoryState == RepositoryState.REBASING_INTERACTIVE ||
                repositoryState == RepositoryState.MERGING || repositoryState == RepositoryState.MERGING_RESOLVED) {
            logger.info("the repo is merging");
        }
        repository=gitUtils.getRepository(path);
        repositoryState = repository.getRepositoryState();

        if (repositoryState!=RepositoryState.MERGING_RESOLVED && repositoryState!=RepositoryState.MERGING) {
            fileInfoMapper.deleteAll(path);
            logger.info("the repo has not been merged-----");
            ProcessBuilder processBuilder = new ProcessBuilder("git", "checkout", b1);
            // 设置工作目录
            processBuilder.directory(new File(path));
            // 启动进程执行git checkout命令
            Process process = processBuilder.start();
            // 等待命令执行完毕
            process.waitFor();
            // 创建一个ProcessBuilder对象
            processBuilder = new ProcessBuilder("git", "merge", b2, "-s", "recursive", "-X", "patience", "--no-edit", "--no-ff");
            logger.info("execute Command: " + String.join(" ", processBuilder.command()));
            // 设置工作目录
            processBuilder.directory(new File(path));
            // 启动进程执行git merge命令
            process = processBuilder.start();
            // 获取命令执行结果
            int exitValue = process.waitFor();

            if (exitValue != 0) {
                // 命令执行失败，可能有base的情况
                // 处理标准错误流
                InputStream errorStream = process.getErrorStream();
                BufferedReader errorReader = new BufferedReader(new InputStreamReader(errorStream));
                String errorLine;
                while ((errorLine = errorReader.readLine()) != null) {
                    logger.info("repoError: " + errorLine);
                    if(errorLine.contains("fatal: refusing to merge unrelated histories")){
                        // 执行no base merge: --allow-unrelated-histories
                        processBuilder = new ProcessBuilder("git", "merge", b2, "-s", "recursive", "-X", "patience", "--no-edit", "--no-ff","--allow-unrelated-histories");
                        // 设置工作目录
                        processBuilder.directory(new File(path));
                        // 启动进程执行git merge命令
                        process = processBuilder.start();
                        // 获取命令执行结果
                        process.waitFor();
                    }
                }

            }
        }
        repository.close();
    }

    //ret: MERGING_RESOLVED or SAFE
    @Override
    public String commitAll(String path,String message) throws Exception {
        GitUtils gitUtils=new GitUtils();
        Repository repository = gitUtils.getRepository(path);
        RepositoryState repositoryState = repository.getRepositoryState();
        if(repositoryState.toString().equals("SAFE")){
            //git commit --amend 命令用于修改最后一次提交的提交消息。这个命令的目的是允许你修改最后一次提交的提交消息，而不会更改提交的内容
            try (Git git = new Git(repository)) {
                git.add().addFilepattern(".").call();
                git.commit()
                        .setAmend(true) // 指示要执行的提交是一个修改操作
                        .setMessage(message)
                        .call();
            }
            repository.close();
            fileInfoMapper.deleteAll(path);
            return repositoryState.toString();
        }
//        System.out.println("repositoryState mergeBranch commitAll:"+repositoryState);

        try (Git git = new Git(repository)) {
            git.add().addFilepattern(".").call();
            git.commit()
                    .setMessage(message)
                    .call();
        }
        repository.close();
        fileInfoMapper.deleteAll(path);
        return repositoryState.toString();
    }

    //ret: MERGING_RESOLVED or SAFE
    @Override
    public String commitAllGerrit(String path,String message,String target, String topic) throws Exception {
        String repositoryState = commitAll(path,message);
        // 增加gerrit逻辑，git push origin HEAD:refs/for/目标分支%topic=TOPIC
        // 构建命令和参数
        String[] command = {
                "git",                  // Git 命令
                "push",                 // push 操作
                "origin",               // 远程仓库名
                "HEAD:refs/for/" + target + "%topic=" + topic  // 分支和特定格式的提交信息
        };
        logger.info("execute Command: " + String.join(" ", command));
        try {
            // 创建 ProcessBuilder，并设置工作目录
            ProcessBuilder processBuilder = new ProcessBuilder(command);
            processBuilder.directory(new File(path));
            processBuilder.redirectErrorStream(true);
            Process process = processBuilder.start();

            // 读取进程输出
            BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
            StringBuilder output = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) {
                output.append(line).append("\n");
            }
            int exitCode = process.waitFor();
            if (exitCode == 0) {
                // 进程成功执行
                logger.info("Git push to Gerrit succeeded.");
            } else {
                // 进程执行失败，输出错误信息
                logger.info("Git push to Gerrit failed. Error: " + output);
            }
        }catch (Exception e) {
            logger.info("execute Command failure");
            return "GerritExeFail";
        }
        return repositoryState;
    }

    @Override
    public void reset(String repoPath, String fileName) throws Exception {
        GitUtils gitUtils=new GitUtils();
        PathUtils pathUtils=new PathUtils();
        Repository repository = gitUtils.getRepository(repoPath);
        fileName=pathUtils.replaceSep(fileName);
        try (Git git = new Git(repository)) {
            git.checkout().addPath(fileName).call();
        }
        File file=new File(fileName);
        FileInfoWithBlobs infoWithBLOBs=fileInfoMapper.selectByPrimaryKey(pathUtils.getFileWithPathSegment(repoPath,fileName));
        infoWithBLOBs.setIssolve(1);
        fileInfoMapper.updateByPrimaryKeySelective(infoWithBLOBs);

        repository.close();
    }


}
