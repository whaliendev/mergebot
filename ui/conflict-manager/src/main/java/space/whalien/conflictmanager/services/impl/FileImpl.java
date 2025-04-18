package space.whalien.conflictmanager.services.impl;

import org.eclipse.jgit.api.CheckoutCommand;
import org.eclipse.jgit.api.Git;
import org.eclipse.jgit.lib.Repository;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import space.whalien.conflictmanager.dao.FileInfoMapper;
import space.whalien.conflictmanager.dao.SolveMapper;
import space.whalien.conflictmanager.pojo.FileInfoWithBlobs;
import space.whalien.conflictmanager.pojo.MergeTuple;
import space.whalien.conflictmanager.pojo.Solved;
import space.whalien.conflictmanager.pojo.vo.FileTree;
import space.whalien.conflictmanager.services.ConflictService;
import space.whalien.conflictmanager.services.FileService;
import space.whalien.conflictmanager.utils.FileUtils;
import space.whalien.conflictmanager.utils.GitUtils;
import space.whalien.conflictmanager.utils.PathUtils;
import space.whalien.conflictmanager.utils.ScoreUtils;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

@Service
public class FileImpl implements FileService {

    @Autowired
    ConflictService conflictService;
    @Autowired
    SolveMapper solveMapper;
    @Autowired
    FileInfoMapper infoMapper;
    private static final Logger logger = LoggerFactory.getLogger(ConflictImpl.class);

    /**
     * 遍历路径下的所有文件夹
     *
     * @Param filePath 需要遍历的文件夹路径
     * @Return
     */
    @Override
    public List<Object> getAllDirection(String dirPath) {
        List<Object> directionList = new ArrayList<>();
        File baseFile = new File(dirPath);
        if (!baseFile.isFile()) {
            baseFile.exists();
        }
        File[] files = baseFile.listFiles();
        for (File file : files) {
            if (file.isDirectory()) {
                directionList.add(file.getAbsolutePath());
            }
        }
        return directionList;
    }

    /**
     * 遍历该路径下的所有文件夹内的文件，包括子文件夹内的文件
     *
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
            if (file.isFile()) {
                Map<String, String> map = new HashMap<>();
                map.put("fileName", file.getName());
                map.put("filePath", file.getAbsolutePath());
                list.add(map);
            }
        }
        return list;
    }


    @Override
    public List<FileTree> getAllFiles(String directoryPath, String repoPath, List<FileInfoWithBlobs> conflictFiles, List<FileInfoWithBlobs> allFiles) {
        List<FileTree> list = new ArrayList<>();
        PathUtils pathUtils = new PathUtils();
        FileUtils fileUtils = new FileUtils();
        File baseFile = new File(directoryPath);
        if (baseFile.isFile() || !baseFile.exists()) {
            return list;
        }
        if (conflictFiles == null && allFiles == null) {
            conflictFiles = infoMapper.getUnresolved(repoPath);
            allFiles = infoMapper.getAllFiles(repoPath);
        }
        File[] files = baseFile.listFiles();
        for (File file : files) {
            if (file.isFile()) {
                FileTree fileTree = new FileTree(file.getName(),
                        file.getAbsolutePath(),
                        pathUtils.getRelativePath(repoPath, file.getPath()),
                        "file",
                        0,
                        0,
                        null,
                        1);
                for (FileInfoWithBlobs eachFile : allFiles) {
                    if (eachFile.getPath().equals(file.getAbsolutePath())) {
                        if (eachFile.getIssolve() == 1) {
                            fileTree.isConflictFile = 1;
                        } else {
                            fileTree.isConflictFile = 2;
                        }
                        if (!fileUtils.isBinaryFile(file.getAbsolutePath())) {
                            fileTree.isBinary = 0;
                        }
                    }
                }
                list.add(fileTree);
            }
            if (file.isDirectory()) {
                FileTree fileTree = new FileTree(file.getName(),
                        file.getAbsolutePath(),
                        pathUtils.getRelativePath(repoPath, file.getPath()),
                        "direction",
                        0,
                        0,
                        getAllFiles(file.getAbsolutePath(), repoPath, conflictFiles, allFiles),
                        0);
                int i = 0;
                for (FileTree tree : fileTree.childTree) {
                    if (tree.fileType.equals("direction")) {
                        i += tree.conflictNumber;
                    } else if (tree.isConflictFile == 1) {
                        i++;
                    }
                }
                fileTree.conflictNumber = i;
                list.add(fileTree);
            }
        }
        return list;
    }

    @Override
    public void write2ConflictFile(String content, String path, String fileName, String repo, String tempPath) {
        PrintStream stream;
        FileUtils fileUtils = new FileUtils();
        PathUtils pathUtils = new PathUtils();
        ScoreUtils scoreUtils = new ScoreUtils();
        try {
            stream = new PrintStream(path);
            stream.print(content);
            stream.close();
            FileInfoWithBlobs infoWithBLOBs = infoMapper.selectByPrimaryKey(pathUtils.getSystemCompatiblePath(path));
            if (infoWithBLOBs != null && !tempPath.equals("-1")) {
                infoWithBLOBs.setIssolve(0);
                infoMapper.updateByPrimaryKeySelective(infoWithBLOBs);
                List<String> conflict = fileUtils.readFile(new File(pathUtils.getSystemCompatiblePath(pathUtils.getFileWithPathSegment(tempPath, "conflict.java"))));
                org.apache.commons.io.FileUtils.deleteDirectory(new File(tempPath));
                File res = new File(path);
                List<String> resolve = fileUtils.readFile(res);
                List<MergeTuple> tuples = conflictService.extractTuple(conflict, resolve, 1);
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
    }

    @Override
    public List<FileTree> filterFiles(String path) {
        throw new UnsupportedOperationException("method [space.whalien.conflictmanager.services.impl.FileImpl.filterFiles] is not implemented");
    }


    @Override
    public int chooseBinaryFile(String repoPath, String fileName, int branch) throws Exception {
        //filename->f/subf/4f169cf1-ebce-4f9d-a475-4f8ba161b631.png

        GitUtils gitUtils = new GitUtils();
        PathUtils pathUtils = new PathUtils();
        Repository repository = gitUtils.getRepository(repoPath);
//
//        RepositoryState repositoryState = repository.getRepositoryState();
//        System.out.println("chooseBinaryFile repositoryState 1:"+repositoryState);

        try (Git git = new Git(repository)) {
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
            FileInfoWithBlobs infoWithBLOBs = infoMapper.selectByPrimaryKey(pathUtils.getFileWithPathSegment(repoPath, pathUtils.replaceSep(fileName)));
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
        FileUtils fileUtils = new FileUtils();
        PathUtils pathUtils = new PathUtils();
        List<String> fileContent = null;
        if (filePath != null) {
            fileContent = fileUtils.readFile(new File(pathUtils.getSystemCompatiblePath(filePath)));
        }
        return fileContent;
    }

    @Override
    public void renameFileAndUpdateTree(String filePath, String newFileName, String repoPath) throws FileNotFoundException {
        PathUtils pathUtils = new PathUtils();
        File file = new File(filePath);
        //1.判断文件
        if (!file.exists()) {
            throw new FileNotFoundException("文件不存在");
        }
        //2.文件
        if (file.isFile()) {
            FileInfoWithBlobs info = infoMapper.selectByPrimaryKey(filePath);
            String newPath = file.getParent() + File.separator + newFileName;
            if (file.renameTo(new File(newPath))) {
                if (info != null) {
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
            List<FileInfoWithBlobs> infos = infoMapper.getAllFiles(pathUtils.getSystemCompatiblePath(repoPath));
            if (file.renameTo(new File(newPath))) {
                for (FileInfoWithBlobs fileInfo : infos) {
//                    logger.info("ours"+ fileInfo.getOurs());
//                    logger.info("theirs"+ fileInfo.getTheirs());
                    String oldPath = fileInfo.getPath();
                    if (oldPath.startsWith(filePath)) {
                        String newSonPath = oldPath.replace(filePath, newPath);
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
    public void addFileAndUpdateTree(String filePath, String fileType) throws IOException {
        try {
            Path path = Paths.get(filePath);
            if (fileType.equals("file")) {
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
        File newFile = new File(filePath);
        if (newFile.exists()) {
            if (newFile.isFile()) {
                FileInfoWithBlobs info = infoMapper.selectByPrimaryKey(filePath);
                if (info != null) {
                    infoMapper.deleteByPrimaryKey(filePath);
                }
                newFile.delete();
            }
            if (newFile.isDirectory()) {
                List<FileInfoWithBlobs> infos = infoMapper.getAllFiles(repoPath);
                for (FileInfoWithBlobs info : infos) {
                    String oldPath = info.getPath();
                    if (oldPath.startsWith(filePath)) {
                        infoMapper.deleteByPrimaryKey(oldPath);
                    }
                }
                org.apache.commons.io.FileUtils.deleteDirectory(newFile);
            }

        }
    }
}
