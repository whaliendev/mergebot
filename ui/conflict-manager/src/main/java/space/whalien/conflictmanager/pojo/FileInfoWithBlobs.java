package space.whalien.conflictmanager.pojo;

public class FileInfoWithBlobs {
    private String filename;

    private String path;

    private String relPath;

    private Integer issolve;

    private String ours;

    private String theirs;

    private String base;

    private String repo;

    public FileInfoWithBlobs(String file, int issolve) {
        this.filename=file;
        this.issolve=issolve;
    }
    public FileInfoWithBlobs() {}

    public void setFilename(String filename) {
        this.filename = filename == null ? null : filename.trim();
    }

    public String getPath() {
        return path;
    }

    public void setPath(String path) {
        this.path = path == null ? null : path.trim();
    }

    public String getRelPath() {
        return relPath;
    }

    public void setRelPath(String relPath) {
        this.relPath = relPath == null ? null : relPath.trim();
    }

    public Integer getIssolve() {
        return issolve;
    }

    public void setIssolve(Integer issolve) {
        this.issolve = issolve;
    }

    public String getOurs() {
        return ours;
    }

    public void setOurs(String ours) {
        this.ours = ours;
    }

    public String getTheirs() {
        return theirs;
    }

    public void setTheirs(String theirs) {
        this.theirs = theirs;
    }

    public String getBase() {
        return base;
    }

    public void setBase(String base) {
        this.base = base;
    }
    public String getFilename() {
        return filename;
    }

    public String getRepo() {
        return repo;
    }

    public void setRepo(String repo) {
        this.repo = repo;
    }
}
