package space.whalien.conflictmanager.controller;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;
import space.whalien.conflictmanager.dao.FileInfoMapper;
import space.whalien.conflictmanager.dao.SolveMapper;
import space.whalien.conflictmanager.pojo.Solved;

import java.util.List;

@CrossOrigin
@RestController
public class ControlController {
    @Autowired
    private FileInfoMapper fileInfoMapper;
    @Autowired
    private SolveMapper mapper;

    @DeleteMapping(value = "/delete")
    public int deleteAll(@RequestParam("repo") String repo) {
        fileInfoMapper.deleteAll(repo);
        return 1;
    }

    @GetMapping(value = "/search")
    public List<Solved> search() {
        return mapper.searchResolved("public static class RejectAllFilter extends Filter");
    }

    @GetMapping("/health")
    public String health() {
        return "OK";
    }
}
