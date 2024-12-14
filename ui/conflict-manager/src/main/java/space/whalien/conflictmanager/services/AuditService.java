package com.example.filemanager.services;

import com.alibaba.fastjson.JSON;
import com.example.filemanager.dao.AuditFileMapper;
import com.example.filemanager.dao.ResolutionChoiceMapper;
import com.example.filemanager.dao.FileInfoMapper;
import com.example.filemanager.pojo.AuditFile;
import com.example.filemanager.pojo.ResolutionChoice;
import com.example.filemanager.pojo.FileInfoWithBlobs;
import com.example.filemanager.utils.GitUtils;
import com.example.filemanager.utils.PathUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.lang.NonNull;
import org.springframework.stereotype.Service;

import java.io.IOException;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

@Service
public class AuditService {
    private static final Logger logger = LoggerFactory.getLogger(AuditService.class);
    private final RedisService redisService;
    private final AuditFileMapper auditFileMapper;
    private final ResolutionChoiceMapper resolutionChoiceMapper;
    private final FileInfoMapper infoMapper;


    private static final String FILE_LIKE_PREFIX = "filemanager:like:";
    private static final String FILE_TRACK_PREFIX = "filemanager:track:";

    public AuditService(RedisService redisService, AuditFileMapper auditFileMapper, ResolutionChoiceMapper resolutionChoiceMapper, FileInfoMapper infoMapper) {
        this.redisService = redisService;
        this.auditFileMapper = auditFileMapper;
        this.resolutionChoiceMapper = resolutionChoiceMapper;
        this.infoMapper = infoMapper;
    }

    // to redis
    public Boolean thumbUpFile(@NonNull String fileMD5) {
        return redisService.setString(FILE_LIKE_PREFIX + fileMD5, "1");
    }

    public Boolean thumbDownFile(String fileMD5) {
        return redisService.deleteKey(FILE_LIKE_PREFIX + fileMD5);
    }

    public Boolean saveBlockResolutionChoice(String fileMD5, Integer blockIdx, ResolutionChoice choice) {
        String choiceStr = JSON.toJSONString(choice);
        return redisService.setIntHash(FILE_TRACK_PREFIX + fileMD5, String.valueOf(blockIdx), choiceStr);
    }

    private ResolutionChoice convertCacheChoiceToDBChoice(Object blockChoice, Long fileId) {
        String choiceStr = (String) blockChoice;
        ResolutionChoice choice = JSON.parseObject(choiceStr, ResolutionChoice.class);
        choice.setFileId(fileId);
        return choice;
    }

    public Boolean saveFileTrackingData(String fileMD5, String projectPath, String targetBranch,
                                        String sourceBranch, String fileName,
                                        Integer blockCnt) throws IOException {
        boolean liked = false;
        String likeStr = redisService.getString(FILE_LIKE_PREFIX + fileMD5);
        if ("1".equals(likeStr)) {
            liked = true;
        }
        String targetHash = GitUtils.getFullHash(targetBranch, projectPath);
        String sourceHash = GitUtils.getFullHash(sourceBranch, projectPath);
        AuditFile file = new AuditFile(-1L, projectPath, targetHash, sourceHash, fileName, liked);
        AuditFile duplicatedFile = auditFileMapper.selectByMergeScenarioAndFileName(projectPath, targetHash, sourceHash, fileName);
        if (duplicatedFile != null) {
            logger.warn("Duplicated file: {}", duplicatedFile);
            // if file already exists, delete it and clear resolution_choice table, transaction?
            auditFileMapper.deleteAuditFile(duplicatedFile.getId());
            resolutionChoiceMapper.deleteByFileId(duplicatedFile.getId());
        }
        Integer insertResult = auditFileMapper.insertAuditFile(file);
        if (insertResult == 0) {
            logger.error("Failed to insert audit file: {}", file);
            return false;
        }
        // success, delete like status of this file. TODO(hwa): handling returned status
        redisService.deleteKey(FILE_LIKE_PREFIX + fileMD5);

        // merge conflicts have been resolved, just like no blockChoices
        if(blockCnt==0) {
            return true;
        }

        Long fileId = file.getId();
        Map<Integer, Object> trackingData = redisService.getIntHash(FILE_TRACK_PREFIX + fileMD5);
        if (trackingData == null) {
            logger.info("No tracking data for file: {}", Paths.get(projectPath, fileName));
            trackingData = new HashMap<>();
        }

        List<ResolutionChoice> choices = new ArrayList<>();
        for (int i = 0; i < blockCnt; ++i) {
            boolean cached = trackingData.containsKey(i);
            ResolutionChoice choice;
            if (cached) {
                choice = convertCacheChoiceToDBChoice(trackingData.get(i), fileId);
            } else {
                choice = new ResolutionChoice(-1L, fileId, i, "manual", 0f, "");
            }
            choices.add(choice);
        }
        insertResult = resolutionChoiceMapper.insertBatchResolutionChoice(choices);
        if (insertResult == 0) {
            logger.error("Failed to insert resolution choices: {}", choices);
            return false;
        }
        // TODO(hwa): handling returned status
        redisService.deleteHash(FILE_TRACK_PREFIX + fileMD5);

        return true;
    }


    public String getRelPath(String path) {
        PathUtils pathUtils=new PathUtils();
        FileInfoWithBlobs infoWithBLOBs=infoMapper.selectByPrimaryKey(pathUtils.getSystemCompatiblePath(path));
        return infoWithBLOBs.getRelPath();
    }
}
