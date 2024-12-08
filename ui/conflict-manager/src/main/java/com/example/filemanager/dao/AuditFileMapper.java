package com.example.filemanager.dao;

import com.example.filemanager.pojo.AuditFile;
import org.apache.ibatis.annotations.Mapper;
import org.springframework.stereotype.Repository;

@Mapper
@Repository
public interface AuditFileMapper {
    Integer insertAuditFile(AuditFile file);

    Integer updateAuditFile(AuditFile file);

    Integer deleteAuditFile(Long id);

    AuditFile selectByPrimaryKey(Long id);

    AuditFile selectByMergeScenarioAndFileName(String projectPath, String targetBranch, String sourceBranch, String fileName);
}
