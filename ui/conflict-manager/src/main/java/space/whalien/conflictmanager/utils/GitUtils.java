package space.whalien.conflictmanager.utils;


import org.eclipse.jgit.lib.ObjectId;
import org.eclipse.jgit.lib.Ref;
import org.eclipse.jgit.lib.Repository;
import org.eclipse.jgit.revwalk.RevCommit;
import org.eclipse.jgit.revwalk.RevTag;
import org.eclipse.jgit.revwalk.RevWalk;
import org.eclipse.jgit.storage.file.FileRepositoryBuilder;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class GitUtils {

    public Repository getRepository(String path) throws Exception {
        File gitDirectory = new File(path);
        Repository repository;
        FileRepositoryBuilder builder = new FileRepositoryBuilder();
        repository = builder.setGitDir(new File(gitDirectory, ".git"))
                .readEnvironment()
                .findGitDir()
                .build();
        return repository;
    }

    public List<RevCommit> getMergeCommits(Repository repository) throws Exception {
        List<RevCommit> commits = new ArrayList<>();
        try (RevWalk revWalk = new RevWalk(repository)) {
            for (Ref ref : repository.getRefDatabase().getRefs()) {
                revWalk.markStart(revWalk.parseCommit(ref.getObjectId()));
            }
            for (RevCommit commit : revWalk) {
                if (commit.getParentCount() == 2) {
                    commits.add(commit);
                }
            }
        }
        return commits;
    }

    //根据id获取指定的commit
    public RevCommit getSpecificCommit(Repository repository, ObjectId id) throws Exception {
        try (RevWalk revWalk = new RevWalk(repository)) {
            return revWalk.parseCommit(id);
        } catch (IllegalArgumentException e) {
            // Thrown if the ObjectId does not correspond to a commit
            System.err.println("The provided ObjectId does not correspond to a commit: " + id);
            return null;
        }
    }

    public static String getFullHash(String revision, String projectPath) throws IOException {
        try (Repository repository = new FileRepositoryBuilder()
                .setGitDir(new File(projectPath, ".git"))
                .build();
             RevWalk revWalk = new RevWalk(repository)) {

            ObjectId objectId = repository.resolve(revision);
            if (objectId == null) {
                throw new IllegalArgumentException("Could not resolve revision: " + revision);
            }

            // Attempt to load the object as a commit or a tag
            ObjectId commitId;
            try {
                if (repository.getObjectDatabase().has(objectId)) {
                    RevCommit commit = revWalk.parseCommit(objectId);
                    commitId = commit.getId();
                } else {
                    // If the object is not a commit, try parsing it as a tag
                    RevTag tag = revWalk.parseTag(objectId);
                    commitId = tag.getObject().getId();
                }
            } catch (IOException e) {
                throw new IOException("Failed to parse object: " + objectId.getName(), e);
            }

            return commitId.getName();
        }
    }
}
