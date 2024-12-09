package com.example.filemanager.controller;

import com.example.filemanager.dao.fileInfoMapper;
import com.example.filemanager.dao.SolveMapper;
import com.example.filemanager.pojo.Solved;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@CrossOrigin
@RestController
public class ControlController {
    @Autowired
    private fileInfoMapper fileInfoMapper;
    @Autowired
    private SolveMapper mapper;

    @DeleteMapping(value ="/delete")
    public int deleteAll(@RequestParam("repo") String repo){
        fileInfoMapper.deleteAll(repo);
        return  1;
    }


    @GetMapping(value = "/search")
    public List<Solved> search(){
        return mapper.searchResolved("public static class RejectAllFilter extends Filter");
    }

    @GetMapping("/health")
    public String health(){
        return "OK";
    }
}
