package space.whalien.conflictmanager.dao;

import org.apache.ibatis.annotations.Mapper;
import org.springframework.stereotype.Repository;
import space.whalien.conflictmanager.pojo.Solved;

import java.util.List;

@Mapper
@Repository
public interface SolveMapper {

    List<Solved> searchResolved(String tuple);
    int insert(Solved solve);

    List<Solved> searchHistory(String path);

    int updateByChunk(Solved record);
}
