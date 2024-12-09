package com.example.filemanager.controller;

import com.example.filemanager.dao.fileInfoMapper;
import com.example.filemanager.pojo.MergeScenario;
import com.example.filemanager.pojo.MergeTuple;
import com.example.filemanager.pojo.fileInfoWithBLOBs;
import com.example.filemanager.services.ConflictServices;
import com.example.filemanager.services.impl.ConflictImpl;

import org.apache.ibatis.jdbc.Null;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.client.HttpClientErrorException;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

@CrossOrigin
@RestController
public class ConflictController {
    @Autowired
    ConflictServices conflictServices;
    @Autowired
    private fileInfoMapper fileInfoMapper;

    private static final Logger logger = LoggerFactory.getLogger(ConflictController.class);

/**
 * 2022-11-12
 不使用，仅示范用
 * @Param path:本地仓库路径
 * @Return data:
 * mergeScenarios base,ours,theirs以string方式存储的文件内容，
 * conflict 以list<String>方式按行存储的冲突文件内容，
 * fileName 冲突文件名 ，
 * absPath 绝对路径，本地访问路径
 *
 */
    @GetMapping(value="/conflicts")
    public Object getConflicts(@RequestParam("path") String path){
        Map<String, Object> dataMap = new HashMap<>();
        try {
            List<MergeScenario> mergeScenarios=conflictServices.getAllConflictInfo(path);
            dataMap.put("data", mergeScenarios);
            dataMap.put("code", 200);
            dataMap.put("msg", "query conflicts successfully");
        } catch (Exception e) {
            e.printStackTrace();
            dataMap.put("data", "");
            dataMap.put("code", 500);
            dataMap.put("root", "");
            dataMap.put("msg", "query conflicts failed");
        }
        return dataMap;
    }
//    /**
//     * 不使用
//     * 2022-11-19
//     * @Param path:本地仓库路径
//     * @Param branch1:主分支名
//     * @Param branch2:需要合并的分支名
//     * @Return data:指定分支合并后得到的冲突文件内容
//     * mergeScenarios base,ours,theirs:以string方式存储的文件内容，
//     * conflict:以list<String>方式按行存储的冲突文件内容，
//     * fileName:冲突文件名 ，
//     * absPath:绝对路径，本地访问路径
//     *
//     */
//    @GetMapping(value="/specifiedConflicts")
//    public Object getSpecifiedConflicts(@RequestParam("path") String path,
//                                        @RequestParam("branch1") String branch1,
//                                        @RequestParam("branch2") String branch2){
//        Map<String, Object> dataMap = new HashMap<>();
//        try {
//            List<MergeScenario> mergeScenarios=conflictServices.getSpecifiedConflictInfo(path,branch1,branch2);
//            dataMap.put("data", mergeScenarios);
//            dataMap.put("code", 200);
//            dataMap.put("msg", "成功收集冲突");
//        } catch (Exception e) {
//            e.printStackTrace();
//            dataMap.put("code", 500);
//            dataMap.put("msg", "查询失败");
//        }
//        return dataMap;
//    }
/**
 * 2022-11-22
 * path:本地仓库路径
 * branch1:主分支名
 * branch2:需要合并的分支名
 * 提取冲突，并将冲突文件存储到数据库中
 */
    @PutMapping(value="/conflict/collect")
    public Object updateUser(@RequestParam("path") String path,
                          @RequestParam("target") String target,
                          @RequestParam("source") String source){
        Map<String, Object> dataMap = new HashMap<>();
        try {
            conflictServices.saveMergeInfo(path, target, source);
            dataMap.put("code", 200);
            dataMap.put("msg", "collect conflicts and save to db successfully");
        } catch (Exception e) {
            e.printStackTrace();
            dataMap.put("code", 500);
            dataMap.put("msg", "collect conflicts and save to db failed");
        }
        return dataMap;
    }
    /**
     * 2023-1-6
     * fileName:需要提取的文件名
     * 提取指定的文件及冲突内容
     * 同时会提取指定冲突文件中的冲突块，并通过这些冲突块内容搜索已经解决的冲突解决方案，并提供参考
     * tuples中的historyTruth为推荐的冲突解决方案，目前为3个
     */
    @GetMapping(value="/conflict/specified")
    public Object getSpecifiedFile(@RequestParam("filePath") String filePath,
                                   @RequestParam("repo") String repo,
                                   @RequestParam("fileType") int isBinary){
        Map<String, Object> dataMap = new HashMap<>();
        try {
            MergeScenario mergeScenario=conflictServices.getSpecifiedFile(filePath,repo,isBinary);
            List<MergeTuple> tuples=new ArrayList<>();
            if(isBinary==0) {
                tuples = conflictServices.extractTuple(mergeScenario.conflict, null, 0);
//                int[] auto = conflictServices.detectAutoMerge(mergeScenario);
//                dataMap.put("diffInfo",auto);
            }
            dataMap.put("data", mergeScenario);
            dataMap.put("info",tuples);
            dataMap.put("code", 200);
            dataMap.put("msg", "read the conflicting file successfully");
        } catch (Exception e) {
            e.printStackTrace();
            dataMap.put("data", "");
            dataMap.put("code", 500);
            dataMap.put("root", "");
            dataMap.put("msg", "fail to read the conflicting file");
        }
        return dataMap;
    }

    @GetMapping(value = "/conflict/all")
    public Object getAll(@RequestParam("repo") String repo){
        Map<String, Object> dataMap = new HashMap<>();
        try {
            List<fileInfoWithBLOBs> fileInfos=fileInfoMapper.getUnsolved(repo);
            dataMap.put("data",fileInfos);
            dataMap.put("code", 200);
            dataMap.put("msg", "query conflicts info successfully");
        } catch (Exception e) {
            e.printStackTrace();
            dataMap.put("code", 500);
            dataMap.put("msg", "query conflicts info failed");
        }
        return dataMap;
    }

    @GetMapping(value = "/branch")
    public Object getBranch(@RequestParam("path") String path){
        logger.info("get branch of repo {}", path);
        Map<String, Object> dataMap = new HashMap<>();
        try {
            List<String> branches=conflictServices.getBranch(path);
            dataMap.put("data",branches);
            dataMap.put("code", 200);
            dataMap.put("msg", "query branches successfully");
        } catch (Exception e) {
            e.printStackTrace();
            dataMap.put("code", 500);
            dataMap.put("msg", "query branches failed");
        }
        return dataMap;

    }

    @GetMapping(value = "/conflict/number")
    public Object getNumber(@RequestParam("repo") String repo){
        Map<String, Object> dataMap = new HashMap<>();
        try {
            int i=conflictServices.checkFinish(repo);
            dataMap.put("data",i);
            dataMap.put("code", 200);
            dataMap.put("msg", "query conflict number successfully");
        } catch (Exception e) {
            e.printStackTrace();
            dataMap.put("code", 500);
            dataMap.put("msg", "query failed");
        }
        return dataMap;
    }
}
