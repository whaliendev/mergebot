package com.example.filemanager.dao;

import com.example.filemanager.pojo.FileInfoWithBlobs;
import org.apache.ibatis.annotations.Mapper;
import org.springframework.stereotype.Repository;

import java.util.List;

@Mapper
@Repository
public interface FileInfoMapper {
    int deleteByPrimaryKey(String filename);

    int insert(FileInfoWithBlobs record);

    int insertSelective(FileInfoWithBlobs record);

    FileInfoWithBlobs selectByPrimaryKey(String path);

    int updateByPrimaryKeySelective(FileInfoWithBlobs record);

    int updateByPrimaryKeyWithBLOBs(FileInfoWithBlobs record);

    List<FileInfoWithBlobs> getUnsolved(String repo);
    List<FileInfoWithBlobs> getSolved(String repo);
    List<FileInfoWithBlobs> getAllFiles(String repo);

    int deleteAll(String repo);
}
