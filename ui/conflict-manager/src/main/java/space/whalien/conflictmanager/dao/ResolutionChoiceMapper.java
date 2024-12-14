package com.example.filemanager.dao;

import com.example.filemanager.pojo.ResolutionChoice;
import org.apache.ibatis.annotations.Mapper;
import org.springframework.stereotype.Repository;

import java.util.List;

@Mapper
@Repository
public interface ResolutionChoiceMapper {
    Integer insertBatchResolutionChoice(List<ResolutionChoice> resolutionChoices);

    List<ResolutionChoice> selectByFileId(Long fileId);

    Integer deleteByFileId(Long fileId);
}
