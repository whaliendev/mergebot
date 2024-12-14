package com.example.filemanager.utils;

import java.io.File;
import java.nio.file.Path;
import java.nio.file.Paths;

public class PathUtils {
    public String getFileWithPathSegment(String... segments){
        return getSystemCompatiblePath(String.join(File.separator,segments));
    }
    public String getSystemCompatiblePath(String path) {
        String compatiblePath;
        if (File.separator.equals("/")) {
            compatiblePath = path.replace('\\', '/');
        } else {
            compatiblePath = path.replace('/', '\\');
        }
        return compatiblePath;
    }
    public String replaceSep(String path) {
        String compatiblePath;
            compatiblePath = path.replace('\\', '/');
        return compatiblePath;
    }
    public String getRelativePath(String repoPath, String filePath) {
        Path repo = Paths.get(repoPath);
        Path file = Paths.get(filePath);
        return replaceSep(repo.relativize(file).toString());
    }
}
