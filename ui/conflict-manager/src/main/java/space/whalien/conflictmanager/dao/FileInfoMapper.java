package space.whalien.conflictmanager.dao;

import org.apache.ibatis.annotations.Mapper;
import org.springframework.stereotype.Repository;
import space.whalien.conflictmanager.pojo.FileInfoWithBlobs;

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

    List<FileInfoWithBlobs> getUnresolved(String repo);
    List<FileInfoWithBlobs> getResolved(String repo);
    List<FileInfoWithBlobs> getAllFiles(String repo);

    int deleteAll(String repo);
}
