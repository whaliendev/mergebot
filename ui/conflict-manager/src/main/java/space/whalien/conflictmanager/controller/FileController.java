package space.whalien.conflictmanager.controller;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.multipart.MultipartFile;
import space.whalien.conflictmanager.pojo.vo.FileTree;
import space.whalien.conflictmanager.services.FileService;
import space.whalien.conflictmanager.utils.FileUtils;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.util.HashMap;
import java.util.List;
import java.util.Map;


@CrossOrigin
@RestController
public class FileController {
    @Autowired
    FileService fileService;

    @GetMapping(value = "/file/checkExistence")
    public Object checkFileExistence(@RequestParam("path") String path) {
        Map<String, Object> dataMap = new HashMap<>();
        try {
            boolean exists = FileUtils.isFileExists(path);
            dataMap.put("data", exists);
            dataMap.put("code", 200);
            dataMap.put("msg", exists ? "file exists locally" : "file does not exist locally");
        } catch (Exception e) {
            dataMap.put("data", false);
            dataMap.put("code", 500);
            dataMap.put("msg", e.getMessage());
        }
        return dataMap;
    }


    /**
     * @Param path:指定目录
     * @Return data:目录下所有文件
     * dirList：所有文件名以及绝对路径
     */
    @GetMapping(value = "/files")
    public Object getFiles(@RequestParam("path") String path) {
        Map<String, Object> dataMap = new HashMap<>();
        try {
            List<FileTree> dirList = fileService.getAllFiles(path, path, null, null);
            dataMap.put("data", dirList);
            dataMap.put("code", 200);
            dataMap.put("msg", "query successfully");
        } catch (Exception e) {
            dataMap.put("data", "");
            dataMap.put("code", 500);
            dataMap.put("root", "");
            dataMap.put("msg", e.getMessage());
        }
        return dataMap;

    }

    /**
     * 2023-1-6
     * 写入文件，同时根据冲突文件提取解决方案，存入数据库中待将来解决新冲突时查询
     */
    @PutMapping(value = "/write2file")
    public Object updateUser(@RequestParam("path") String path,
                             @RequestParam("content") String content,
                             @RequestParam("repo") String repo,
                             @RequestParam("fileName") String fileName,
                             @RequestParam("tempPath") String tempPath) {
        Map<String, Object> dataMap = new HashMap<>();
        try {
            fileService.write2ConflictFile(content, path, fileName, repo, tempPath);
            dataMap.put("code", 200);
            dataMap.put("msg", "write to db successfully");
        } catch (Exception e) {
            dataMap.put("code", 500);
            dataMap.put("msg", e.getMessage());
        }
        return dataMap;
    }

    @PutMapping(value = "/files/binary")
    public Object chooseBinary(@RequestParam("path") String path,
                               @RequestParam("fileName") String fileName,
                               @RequestParam("branch") int branch) throws Exception {
        Map<String, Object> dataMap = new HashMap<>();
        try {
            fileService.chooseBinaryFile(path, fileName, branch);
            dataMap.put("code", 200);
            dataMap.put("msg", "Resolve binary conflict successfully");
        } catch (Exception e) {
            dataMap.put("code", 500);
            dataMap.put("msg", e.getMessage());
        }
        return dataMap;

    }

    @GetMapping(value = "/file/content")
    public Object getFileContent(@RequestParam("path") String path) {
        Map<String, Object> dataMap = new HashMap<>();
        try {
            List<String> content = fileService.getFileContent(path);
            dataMap.put("data", content);
            dataMap.put("code", 200);
            dataMap.put("msg", "read th content of the file successfully");
        } catch (Exception e) {
            dataMap.put("data", "");
            dataMap.put("code", 500);
            dataMap.put("root", "");
            dataMap.put("msg", e.getMessage());
        }
        return dataMap;

    }

    /**
     * @Param filePath:指定文件
     * @Param newFileName:新文件名
     * @Param repoPath:仓库路径
     */
    @PutMapping(value = "/files/rename")
    public Object updateFileTree(@RequestParam("filePath") String filePath,
                                 @RequestParam("newFileName") String newFileName,
                                 @RequestParam("repoPath") String repoPath) {
//
        Map<String, Object> dataMap = new HashMap<>();
        try {
            fileService.renameFileAndUpdateTree(filePath, newFileName, repoPath);
            dataMap.put("code", 200);
            dataMap.put("msg", "rename file successfully");
        } catch (Exception e) {
            dataMap.put("data", "");
            dataMap.put("code", 500);
            dataMap.put("root", "");
            dataMap.put("msg", e.getMessage());
        }
        return dataMap;

    }

    @PutMapping(value = "/files/add")
    public Object addFileTree(@RequestParam("filePath") String filePath,
                              @RequestParam("fileType") String fileType) {
//
        Map<String, Object> dataMap = new HashMap<>();
        try {
            fileService.addFileAndUpdateTree(filePath, fileType);
            dataMap.put("code", 200);
            dataMap.put("msg", "add a new file successfully");
        } catch (Exception e) {
            dataMap.put("data", "");
            dataMap.put("code", 500);
            dataMap.put("root", "");
            dataMap.put("msg", e.getMessage());
        }
        return dataMap;

    }

    @PutMapping(value = "/files/delete")
    public Object deleteFileTree(@RequestParam("filePath") String filePath,
                                 @RequestParam("repoPath") String repoPath) {
//
        Map<String, Object> dataMap = new HashMap<>();
        try {
            fileService.deleteFileAndUpdateTree(filePath, repoPath);
            dataMap.put("code", 200);
            dataMap.put("msg", "delete specific file successfully");
        } catch (Exception e) {
            dataMap.put("data", "");
            dataMap.put("code", 500);
            dataMap.put("root", "");
            dataMap.put("msg", e.getMessage());
        }
        return dataMap;

    }

    @PostMapping("/files/upload")
    public Map<String, Object> uploadFile(@RequestParam("file") MultipartFile[] files,
                                          @RequestParam("dirPath") String dirPath) {
        Map<String, Object> dataMap = new HashMap<>();
        try {
            for (MultipartFile file : files) {
                // 在这里处理文件上传逻辑，保存到指定的文件路径 fileSavePath 中
                // 使用file.transferTo(new File(fileSavePath))等方法保存文件
                String filename = file.getOriginalFilename();
                String path = dirPath + File.separator + filename;
                File filePath = new File(path);

                //上传
                BufferedOutputStream outputStream = new BufferedOutputStream(new FileOutputStream(filePath));
                outputStream.write(file.getBytes());
                outputStream.flush();
                outputStream.close();
            }
            //可能需要返回操作失败的文件名数组
            dataMap.put("code", 200);
            dataMap.put("msg", "Upload file successfully");
        } catch (Exception e) {
            //当上传文件超 tomcat 的大小限制后会先于 Controller 触发异常，所以这时我们的异常处理类无法捕获 Controller 层的异常。
            dataMap.put("code", 500);
            dataMap.put("msg", e.getMessage());
        }
        return dataMap;
    }
}
