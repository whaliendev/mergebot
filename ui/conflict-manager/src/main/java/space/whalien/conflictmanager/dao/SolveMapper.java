package com.example.filemanager.dao;

import com.example.filemanager.pojo.Solved;
import org.apache.ibatis.annotations.Mapper;
import org.springframework.stereotype.Repository;

import java.util.List;

@Mapper
@Repository
public interface SolveMapper {

    List<Solved> searchResolved(String tuple);
    int insert(Solved solve);

    List<Solved> searchHistory(String path);

    int updateByChunk(Solved record);
}
