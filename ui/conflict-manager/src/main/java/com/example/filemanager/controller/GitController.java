package com.example.filemanager.controller;

import com.example.filemanager.services.AuditService;
import com.example.filemanager.services.GitServices;
import com.example.filemanager.services.fileServices;
import org.apache.commons.io.filefilter.TrueFileFilter;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.web.bind.annotation.CrossOrigin;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.HashMap;
import java.util.Map;

@CrossOrigin
@RestController
public class GitController {
    @Autowired
    GitServices gitServices;
    @Autowired
    AuditService auditService;
    @Value("${frontend.base-url}")
    private String FrontendBaseUrl;

    @PutMapping(value="/git/merge")
    public Object mergeSpecifiedBranchGerrit(@RequestParam("path") String path,
                                       @RequestParam("target") String target,
                                       @RequestParam("source") String source,
                                             @RequestParam("compdb") String compdb){
        Map<String, Object> dataMap = new HashMap<>();
        try {
            gitServices.mergeBranch(path,target,source);
            dataMap.put("isSuccessful", true);
            dataMap.put("msg", "Merge successfully");
            // 构建URL并添加到dataMap中
            String url = FrontendBaseUrl +"/fileview?repo=" + path + "&target=" + target + "&source=" + source+ "&compdb=" + compdb;
            dataMap.put("data", url);

        } catch (Exception e) {
            e.printStackTrace();
            dataMap.put("isSuccessful", false);
            dataMap.put("msg", "Merge failed");
            dataMap.put("data", "");
        }
        return dataMap;
    }

    @PutMapping(value="/git/commit")
    public Object commit(@RequestParam("path") String path,
                                @RequestParam("message") String message,
                                      @RequestParam("target") String target,@RequestParam("topic") String topic){
        Map<String, Object> dataMap = new HashMap<>();
        try {
            if (!topic.equals("")) {
                String url = "http://gerrit.scm.adc.com:8080/#/q/topic:" + topic;

                //ret: MERGING_RESOLVED or SAFE
                String repositoryState = gitServices.commitAllGerrit(path, message,target,topic);
                if (repositoryState.equals("GerritExeFail")){
                    dataMap.put("isSuccessful", false);
                    dataMap.put("msg", "fail to commit to Gerrit，GerritExeFail");
                }else{
                    dataMap.put("isSuccessful", true);
                    dataMap.put("msg", "commit to Gerrit successfully");
                }
                dataMap.put("repositoryState", repositoryState);
                dataMap.put("data", url);
            }else{
                dataMap.put("isSuccessful", true);
                dataMap.put("msg", "Commit successfully");
                //ret: MERGING_RESOLVED or SAFE
                String repositoryState = gitServices.commitAll(path, message);
                dataMap.put("repositoryState", repositoryState);
            }
        }catch (Exception e) {
            e.printStackTrace();
            dataMap.put("isSuccessful", false);
            dataMap.put("msg", "Merge Failed");
        }
        return dataMap;
    }

    @PutMapping(value="/git/reset")
    public Object resetFile(@RequestParam("repoPath") String path,
                                @RequestParam("filePath") String filePath){
        Map<String, Object> dataMap = new HashMap<>();
        try {
            gitServices.reset(path,auditService.getRelPath(filePath));
            dataMap.put("code", 200);
            dataMap.put("msg", "Git reset successfully");
        } catch (Exception e) {
            e.printStackTrace();
            dataMap.put("code", 500);
            dataMap.put("msg", "Git reset failed");
        }
        return dataMap;
    }
}
