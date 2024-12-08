package com.example.filemanager.dao;

import com.example.filemanager.pojo.fileInfoWithBLOBs;
import org.apache.ibatis.annotations.Mapper;
import org.springframework.stereotype.Repository;

import java.util.List;

@Mapper
@Repository
public interface fileInfoMapper {
    int deleteByPrimaryKey(String filename);

    int insert(fileInfoWithBLOBs record);

    int insertSelective(fileInfoWithBLOBs record);

    fileInfoWithBLOBs selectByPrimaryKey(String path);

    int updateByPrimaryKeySelective(fileInfoWithBLOBs record);

    int updateByPrimaryKeyWithBLOBs(fileInfoWithBLOBs record);

    List<fileInfoWithBLOBs> getUnsolved(String repo);
    List<fileInfoWithBLOBs> getSolved(String repo);
    List<fileInfoWithBLOBs> getAllFiles(String repo);

    int deleteAll(String repo);
}