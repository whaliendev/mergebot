package com.example.filemanager.services.impl;

import com.example.filemanager.dao.SolveMapper;
import com.example.filemanager.dao.fileInfoMapper;
import com.example.filemanager.pojo.MergeScenario;
import com.example.filemanager.pojo.MergeTuple;
import com.example.filemanager.pojo.fileInfoWithBLOBs;
import com.example.filemanager.pojo.Solved;
import com.example.filemanager.pojo.vo.FileTree;
import com.example.filemanager.services.ConflictServices;
import com.example.filemanager.services.fileServices;
import com.example.filemanager.utils.FileUtils;
import com.example.filemanager.utils.GitUtils;
import com.example.filemanager.utils.PathUtils;
import com.example.filemanager.utils.ScoreUtils;
import org.apache.ibatis.jdbc.Null;
import org.checkerframework.checker.units.qual.A;
import org.eclipse.jgit.api.CheckoutCommand;
import org.eclipse.jgit.api.Git;
import org.eclipse.jgit.api.ResetCommand;
import org.eclipse.jgit.api.Status;
import org.eclipse.jgit.api.errors.GitAPIException;
import org.eclipse.jgit.lib.Repository;
import org.eclipse.jgit.lib.RepositoryState;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.expression.spel.ast.NullLiteral;
import org.springframework.stereotype.Service;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;

@Service
public class fileImpl implements fileServices {

    @Autowired
    ConflictServices conflictServices;
    @Autowired
    SolveMapper solveMapper;
    @Autowired
    fileInfoMapper infoMapper;
    private static final Logger logger = LoggerFactory.getLogger(ConflictImpl.class);
/**
 * 遍历路径下的所有文件夹
 * @Param filePath 需要遍历的文件夹路径
 * @Return
 */
    @Override
    public List<Object> getAllDirection(String dirPath) {
        List<Object> directionList = new ArrayList<>();
        File baseFile=new File(dirPath);
        if (!baseFile.isFile()) {
            baseFile.exists();
        }
        File[] files=baseFile.listFiles();
        for (File file:files) {
            if(file.isDirectory()){
                directionList.add(file.getAbsolutePath());
            }
        }
        return directionList;
    }
/**
 * 遍历该路径下的所有文件夹内的文件，包括子文件夹内的文件
 * @Param directoryPath 需要遍历的文件夹路径
 * @Return
 */
    @Override
    public List<Object> getAll(String directoryPath) {
        List<Object> list = new ArrayList<>();
        File baseFile = new File(directoryPath);
        if (baseFile.isFile() || !baseFile.exists()) {
            return list;
        }
        File[] files = baseFile.listFiles();
        for (File file : files) {
            if (file.isDirectory()) {
                list.addAll(getAll(file.getAbsolutePath()));
            }
            if(file.isFile()){
                Map<String, String> map = new HashMap<>();
                map.put("fileName", file.getName());
                map.put("filePath", file.getAbsolutePath());
                list.add(map);
            }
        }
        return list;
    }



    @Override
    public List<FileTree> getAllFiles(String directoryPath,String repoPath,List<fileInfoWithBLOBs> conflictFiles,List<fileInfoWithBLOBs> allFiles) {
        List<FileTree> list = new ArrayList<>();
        PathUtils pathUtils=new PathUtils();
        FileUtils fileUtils=new FileUtils();
        File baseFile = new File(directoryPath);
        if (baseFile.isFile() || !baseFile.exists()) {
            return list;
        }
        if(conflictFiles==null&&allFiles==null) {
            conflictFiles = infoMapper.getUnsolved(repoPath);
            allFiles = infoMapper.getAllFiles(repoPath);
        }
        File[] files = baseFile.listFiles();
        for (File file : files) {
            if (file.isFile()) {
                FileTree fileTree=new FileTree(file.getName(),
                        file.getAbsolutePath(),
                        pathUtils.getRelativePath(repoPath,file.getPath()),
                        "file",
                        0,
                        0,
                        null,
                        1);
                for (fileInfoWithBLOBs eachFile:allFiles) {
                    if(eachFile.getPath().equals(file.getAbsolutePath())) {
                        if(eachFile.getIssolve()==1) {
                            fileTree.isConflictFile = 1;
                        }else{
                            fileTree.isConflictFile = 2;
                        }
                        if (!fileUtils.isBinaryFile(file.getAbsolutePath())) {
                            fileTree.isBinary=0;
                        }
                    }
                }
                list.add(fileTree);
            }
            if (file.isDirectory()) {
//                int i=0;
//                for (fileInfoWithBLOBs eachFile:conflictFiles) {
//                    List<String> dirName;
//                    List<String> eachFileName;
//                    if(File.separator.equals("\\")) {
//                        dirName = Arrays.asList(file.getAbsolutePath().split("\\\\"));
//                        eachFileName = Arrays.asList(eachFile.getPath().split("\\\\"));
//                    }else {
//                        dirName = Arrays.asList(file.getAbsolutePath().split("/"));
//                        eachFileName = Arrays.asList(eachFile.getPath().split("/"));
//                    }
//                    int dirLen=dirName.size();
//                    int fileLen=eachFileName.size()-1;
//                    if(dirLen<=fileLen){
//                        i++;
//                        for(int q=0;q<dirLen;q++){
//                            if(!dirName.get(q).equals(eachFileName.get(q))){
//                               i--;
//                               break;
//                            }
//                        }
//                    }
//                }
                FileTree fileTree=new FileTree(file.getName(),
                        file.getAbsolutePath(),
                        pathUtils.getRelativePath(repoPath,file.getPath()),
                        "direction",
                        0,
                        0,
                        getAllFiles(file.getAbsolutePath(), repoPath,conflictFiles,allFiles),
                        0);
                int i=0;
                for (FileTree tree : fileTree.childTree) {
                    if(tree.fileType.equals("direction")){
                        i+=tree.conflictNumber;
                    }else if(tree.isConflictFile==1){
                        i++;
                    }
                }
                fileTree.conflictNumber=i;
                list.add(fileTree);
            }
        }
        return list;
    }

    @Override
    public void write2ConflictFile(String content, String path,String fileName,String repo,String tempPath) {
//        GitUtils gitUtils=new GitUtils();
//        Repository repository= null;
//        System.out.println("path："+repo);
//
//        try {
//            repository = gitUtils.getRepository(repo);
//        } catch (Exception e) {
//            e.printStackTrace();
//        }
//        RepositoryState repositoryState = repository.getRepositoryState();
//        System.out.println("before write repositoryState*:"+repositoryState);

        PrintStream stream;
        FileUtils fileUtils=new FileUtils();
        PathUtils pathUtils=new PathUtils();
        ScoreUtils scoreUtils=new ScoreUtils();
        try {
            stream=new PrintStream(path);//写入的文件path
            stream.print(content);//写入的字符串
            stream.close();
            fileInfoWithBLOBs infoWithBLOBs=infoMapper.selectByPrimaryKey(pathUtils.getSystemCompatiblePath(path));
            if(infoWithBLOBs!=null&&!tempPath.equals("-1")) {
                infoWithBLOBs.setIssolve(0);
                infoMapper.updateByPrimaryKeySelective(infoWithBLOBs);
//            MergeScenario mergeScenario=conflictServices.getSpecifiedFile(path,repo,0);
                List<String> conflict = fileUtils.readFile(new File(pathUtils.getSystemCompatiblePath(pathUtils.getFileWithPathSegment(tempPath, "conflict.java"))));
                org.apache.commons.io.FileUtils.deleteDirectory(new File(tempPath));
                File res = new File(path);
                List<String> resolve = fileUtils.readFile(res);
                List<MergeTuple> tuples = conflictServices.extractTuple(conflict, resolve, 1);
                for (MergeTuple tuple : tuples) {
                    StringBuilder r = new StringBuilder();
                    StringBuilder c = new StringBuilder();
                    for (String line : tuple.resolve) {
                        r.append(line).append("\n");
                    }
                    for (String line : tuple.ours) {
                        c.append(line).append("\n");
                    }
                    for (String line : tuple.base) {
                        c.append(line).append("\n");
                    }
                    for (String line : tuple.theirs) {
                        c.append(line).append("\n");
                    }
                    if (tuple.resolve.size() > 0) {
                        List<Solved> history = solveMapper.searchHistory(path);
                        int DuNumber = 0;
                        if (history != null) {
                            for (Solved solve1 : history) {
                                if (scoreUtils.checkSimilarity(c.toString(), solve1.chunk) >= 0.95) {
                                    if (DuNumber == 0) {
                                        solve1.solve = r.toString();
                                        solveMapper.updateByChunk(solve1);
                                    }
                                    DuNumber++;
                                }
                            }
                        }
                        if (DuNumber == 0) {
                            Solved solve = new Solved(c.toString(), r.toString(), path);
                            solveMapper.insert(solve);
                        }
                    }
                }
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
//        repositoryState = repository.getRepositoryState();
//        System.out.println("after write repositoryState*:"+repositoryState);
    }

    @Override
    public List<FileTree> filterFiles(String path) {
//        List<FileTree> list = new ArrayList<>();
//        File baseFile = new File(path);
//        if (baseFile.isFile() || !baseFile.exists()) {
//            return list;
//        }
//        List<fileInfoWithBLOBs> conflictFiles=infoMapper.getAllFiles(path);
//        File[] files = baseFile.listFiles();
//        FileUtils fileUtils=new FileUtils();
//        for (File file : files) {
//            if (file.isFile()) {
//                Path path1 = Paths.get(file.getPath());
//                String type;
////                try {
////                    type = fileUtils.getFileType(file.getAbsolutePath());
////                } catch (IOException e) {
////                    throw new RuntimeException(e);
////                }
//                int k1=1;
//                if (!fileUtils.isBinaryFile(file.getAbsolutePath())) {
//                    k1= 0;
//                }
//                int k=0;
//                int isConflict=2;
//                for (fileInfoWithBLOBs eachFile:conflictFiles) {
//                    if(eachFile.getFilename().equals(file.getName())) {
//                        k=1;
//                        if(eachFile.getIssolve()==1){
//                            isConflict=1;
//                        }
//                        break;
//                    }
//                }
//                if(k==1) {
//                    FileTree fileTree = new FileTree(file.getName(), file.getAbsolutePath(), file.getPath(), "file", 0, isConflict, null,k1);
//                    list.add(fileTree);
//                }
//            }
////            if (file.isDirectory()) {
////                int i=0;
////                for (fileInfoWithBLOBs eachFile:conflictFiles) {
////                    if(eachFile.getPath().startsWith(file.getAbsolutePath())) {
////                        i++;
////                        for(int q=0;q<dirLen;q++){
////                            if(!dirName.get(q).equals(eachFileName.get(q))){
////                                i--;
////                                break;
////                            }
////                        }
////                    }
////                }
//                if(i>0) {
//                    FileTree fileTree = new FileTree(file.getName(), file.getAbsolutePath(), file.getPath(), "direction", i, 0, filterFiles(file.getAbsolutePath()),0);
//                    list.add(fileTree);
//                }
//            }
//        }
        return null;
    }


    @Override
    public int chooseBinaryFile(String repoPath, String fileName, int branch) throws Exception {
        //filename->f/subf/4f169cf1-ebce-4f9d-a475-4f8ba161b631.png

        GitUtils gitUtils=new GitUtils();
        PathUtils pathUtils=new PathUtils();
        Repository repository=gitUtils.getRepository(repoPath);
//
//        RepositoryState repositoryState = repository.getRepositoryState();
//        System.out.println("chooseBinaryFile repositoryState 1:"+repositoryState);

        try(Git git=new Git(repository)) {
            CheckoutCommand checkoutCommand = git.checkout();
//            ResetCommand resetCommand = git.reset();
//            resetCommand.addPath(pathUtils.replaceSep(fileName));
//            resetCommand.call();
            if (branch == 1) {
                checkoutCommand.setStage(CheckoutCommand.Stage.OURS).addPath(pathUtils.replaceSep(fileName));
            } else if (branch == 2) {
                checkoutCommand.setStage(CheckoutCommand.Stage.THEIRS).addPath(pathUtils.replaceSep(fileName));
            } else {
                checkoutCommand.setStage(CheckoutCommand.Stage.BASE).addPath(pathUtils.replaceSep(fileName));
            }
            checkoutCommand.call();
            File file = new File(fileName);
            fileInfoWithBLOBs infoWithBLOBs = infoMapper.selectByPrimaryKey(pathUtils.getFileWithPathSegment(repoPath,pathUtils.replaceSep(fileName)));
            infoWithBLOBs.setIssolve(0);
            infoMapper.updateByPrimaryKeySelective(infoWithBLOBs);
        }

//        repositoryState = repository.getRepositoryState();
//        System.out.println("chooseBinaryFile repositoryState 2:"+repositoryState);
        repository.close();
        return 1;
    }

    @Override
    public List<String> getFileContent(String filePath) throws Exception {
        FileUtils fileUtils=new FileUtils();
        PathUtils pathUtils=new PathUtils();
        List<String> fileContent=null;
        if(filePath!=null){
            fileContent=fileUtils.readFile(new File(pathUtils.getSystemCompatiblePath(filePath)));
        }
        return fileContent;
    }

    @Override
    public  void renameFileAndUpdateTree(String filePath, String newFileName,String repoPath) throws FileNotFoundException {
        PathUtils pathUtils=new PathUtils();
        File file = new File(filePath);
        //1.判断文件
        if (!file.exists()) {
            throw new FileNotFoundException("文件不存在");
        }
        //2.文件
        if (file.isFile()) {
            fileInfoWithBLOBs info=infoMapper.selectByPrimaryKey(filePath);
            String newPath = file.getParent() + File.separator + newFileName;
            if (file.renameTo(new File(newPath))) {
                if(info!=null){
                    info.setPath(newPath);
//                    info.setRelPath(pathUtils.getRelativePath(repoPath,newPath));
                    info.setFilename(newFileName);
                    infoMapper.insert(info);
                    infoMapper.deleteByPrimaryKey(filePath);
                }
            }
        }
        //3.文件夹
        if (file.isDirectory()) {
            String newPath = file.getParent() + File.separator + newFileName;
            List<fileInfoWithBLOBs> infos=infoMapper.getAllFiles(pathUtils.getSystemCompatiblePath(repoPath));
            if (file.renameTo(new File(newPath))) {
                for(fileInfoWithBLOBs fileInfo:infos){
//                    logger.info("ours"+ fileInfo.getOurs());
//                    logger.info("theirs"+ fileInfo.getTheirs());
                    String oldPath=fileInfo.getPath();
                    if(oldPath.startsWith(filePath)){
                        String newSonPath=oldPath.replace(filePath,newPath);
//                        String newRelPath=pathUtils.getRelativePath(repoPath,newSonPath);
//                        fileInfo.setRelPath(newRelPath);
                        fileInfo.setPath(newSonPath);
                        infoMapper.deleteByPrimaryKey(oldPath);
                        infoMapper.insert(fileInfo);

                    }
                }
            }
        }
    }

    // 递归更新节点名称
//    private  void updateNodeNameRecursively(List<FileTree> nodeList, String oldPath, String newFileName,String newPath,String repoPath) {
//        PathUtils pathUtils=new PathUtils();
//        for (FileTree node : nodeList) {
//            if (node.filePath.equals(oldPath)) {
//                node.fileName=newFileName;
//                node.filePath=newPath;
//                node.path=pathUtils.getRelativePath(repoPath,node.filePath);
//                break;
//            }
//            if (node.fileType.equals("direction")) {
//                // 对文件夹类型节点进行递归查找
//                updateNodeNameRecursively(node.childTree, oldPath, newFileName,newPath,repoPath);
//            }
//        }
//    }

    @Override
    public void addFileAndUpdateTree(String filePath,String fileType) throws IOException {
        try {
            Path path = Paths.get(filePath);
            if(fileType.equals("file")) {
                Files.createFile(path);
            //            }else if(fileType.equals("direction")){     //TODO: typo of directory，需要改所有的 typo 和文档
            } else {
                Files.createDirectories(path);
            }


        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void deleteFileAndUpdateTree(String filePath, String repoPath) throws IOException {
        File newFile=new File(filePath);
        if(newFile.exists()){
            if(newFile.isFile()){
                fileInfoWithBLOBs info=infoMapper.selectByPrimaryKey(filePath);
                if(info!=null){
                    infoMapper.deleteByPrimaryKey(filePath);
                }
                newFile.delete();
            }
            if(newFile.isDirectory()){
                List<fileInfoWithBLOBs> infos=infoMapper.getAllFiles(repoPath);
                for (fileInfoWithBLOBs info:infos) {
                    String oldPath=info.getPath();
                    if(oldPath.startsWith(filePath)){
                        infoMapper.deleteByPrimaryKey(oldPath);
                    }
                }
                org.apache.commons.io.FileUtils.deleteDirectory(newFile);
            }

        }
    }
}
