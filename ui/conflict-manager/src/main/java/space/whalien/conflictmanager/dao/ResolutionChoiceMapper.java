package space.whalien.conflictmanager.dao;

import org.apache.ibatis.annotations.Mapper;
import org.springframework.stereotype.Repository;
import space.whalien.conflictmanager.pojo.ResolutionChoice;

import java.util.List;

@Mapper
@Repository
public interface ResolutionChoiceMapper {
    Integer insertBatchResolutionChoice(List<ResolutionChoice> resolutionChoices);

    List<ResolutionChoice> selectByFileId(Long fileId);

    Integer deleteByFileId(Long fileId);
}
