import os
from pathlib import Path
from pygit2 import C
from pygit2.errors import check_error
from utils.common import to_bytes
from cffi import FFI

ffi = FFI()

ffi.cdef(
    """
typedef enum {
	GIT_MERGE_FILE_FAVOR_NORMAL = 0,
	GIT_MERGE_FILE_FAVOR_OURS = 1,
	GIT_MERGE_FILE_FAVOR_THEIRS = 2,
	GIT_MERGE_FILE_FAVOR_UNION = 3,
} git_merge_file_favor_t;

typedef enum {
	GIT_MERGE_FILE_DEFAULT = 0,
	GIT_MERGE_FILE_STYLE_MERGE = 1,
	GIT_MERGE_FILE_STYLE_DIFF3 = 2,
	GIT_MERGE_FILE_SIMPLIFY_ALNUM = 4,
	GIT_MERGE_FILE_IGNORE_WHITESPACE = 8,
	GIT_MERGE_FILE_IGNORE_WHITESPACE_CHANGE = 16,
	GIT_MERGE_FILE_IGNORE_WHITESPACE_EOL = 32,
	GIT_MERGE_FILE_DIFF_PATIENCE = 64,
	GIT_MERGE_FILE_DIFF_MINIMAL = 128,
} git_merge_file_flag_t;

typedef struct {
	unsigned int version;
	const char *ancestor_label;
	const char *our_label;
	const char *their_label;
	git_merge_file_favor_t favor;
	git_merge_file_flag_t flags;
	unsigned short marker_size;
} git_merge_file_options;

typedef struct {
	unsigned int version;

	/** Pointer to the contents of the file. */
	const char *ptr;

	/** Size of the contents pointed to in `ptr`. */
	size_t size;

	/** File name of the conflicted file, or `NULL` to not merge the path. */
	const char *path;

	/** File mode of the conflicted file, or `0` to not merge the mode. */
	unsigned int mode;
} git_merge_file_input;

typedef struct {
	unsigned int automergeable;
	const char *path;
	unsigned int mode;
	const char *ptr;
	size_t len;
} git_merge_file_result;

int git_merge_file(
	git_merge_file_result *out,
	const git_merge_file_input *ancestor,
	const git_merge_file_input *ours,
	const git_merge_file_input *theirs,
	const git_merge_file_options *opts);
"""
)

module_path = os.path.abspath(__file__)
git2_path = Path(module_path).parent.parent / "deps" / "libgit2.so.1.7.1"
libgit2 = ffi.dlopen(git2_path.as_posix())

###################
# git_merge_file_favor_t
###################
GIT_MERGE_FILE_FAVOR_NORMAL = C.GIT_MERGE_FILE_FAVOR_NORMAL
GIT_MERGE_FILE_FAVOR_OURS = C.GIT_MERGE_FILE_FAVOR_OURS
GIT_MERGE_FILE_FAVOR_THEIRS = C.GIT_MERGE_FILE_FAVOR_THEIRS
GIT_MERGE_FILE_FAVOR_UNION = C.GIT_MERGE_FILE_FAVOR_UNION

GIT_MERGE_CONFLICT_MARKER_SIZE = 7

###################
# git_merge_file_flag_t
###################
# Defaults
GIT_MERGE_FILE_DEFAULT = C.GIT_MERGE_FILE_DEFAULT
# Create standard conflicted merge files
GIT_MERGE_FILE_STYLE_MERGE = C.GIT_MERGE_FILE_STYLE_MERGE
# Create diff3-style files
GIT_MERGE_FILE_STYLE_DIFF3 = C.GIT_MERGE_FILE_STYLE_DIFF3
# Condense non-alphanumeric regions for simplified diff file
GIT_MERGE_FILE_SIMPLIFY_ALNUM = C.GIT_MERGE_FILE_SIMPLIFY_ALNUM
# Ignore all whitespace
GIT_MERGE_FILE_IGNORE_WHITESPACE = C.GIT_MERGE_FILE_IGNORE_WHITESPACE
# Ignore changes in amount of whitespace
GIT_MERGE_FILE_IGNORE_WHITESPACE_CHANGE = C.GIT_MERGE_FILE_IGNORE_WHITESPACE_CHANGE
# Ignore whitespace at end of line
GIT_MERGE_FILE_IGNORE_WHITESPACE_EOL = C.GIT_MERGE_FILE_IGNORE_WHITESPACE_EOL
# Use the "patience diff" algorithm
GIT_MERGE_FILE_DIFF_PATIENCE = C.GIT_MERGE_FILE_DIFF_PATIENCE
# Take extra time to find minimal diff
GIT_MERGE_FILE_DIFF_MINIMAL = C.GIT_MERGE_FILE_DIFF_MINIMAL
# TODO(hwa): find why
# Create zdiff3 ("zealous diff3")-style files
# GIT_MERGE_FILE_STYLE_ZDIFF3 = C.GIT_MERGE_FILE_STYLE_ZDIFF3
# Do not produce file conflicts when common regions have
# changed; keep the conflict markers in the file and accept
# that as the merge result.
# GIT_MERGE_FILE_ACCEPT_CONFLICTS = C.GIT_MERGE_FILE_ACCEPT_CONFLICTS


class GitMergeFileOptions:
    version = 1
    ancestor_label: str
    our_label: str
    their_label: str
    favor: GIT_MERGE_FILE_FAVOR_NORMAL
    flags: GIT_MERGE_FILE_DEFAULT
    marker_size = GIT_MERGE_CONFLICT_MARKER_SIZE

    def __init__(
        self,
        ancestor_label: str,
        our_label: str,
        their_label: str,
        favor: GIT_MERGE_FILE_FAVOR_NORMAL,
        flags: GIT_MERGE_FILE_DEFAULT,
    ):
        self.ancestor_label = ancestor_label
        self.our_label = our_label
        self.their_label = their_label
        self.favor = favor
        self.flags = flags

    @classmethod
    def from_c(cls, c_options):
        return cls(
            ancestor_label=ffi.string(c_options.ancestor_label).decode(),
            our_label=ffi.string(c_options.our_label).decode(),
            their_label=ffi.string(c_options.their_label).decode(),
            favor=c_options.favor,
            flags=c_options.flags,
        )

    def to_c(self):
        c_options = ffi.new("git_merge_file_options *")
        c_options.version = self.version
        ancestor_label = ffi.new("char[]", to_bytes(self.ancestor_label))
        c_options.ancestor_label = ancestor_label
        our_label = ffi.new("char[]", to_bytes(self.our_label))
        c_options.our_label = our_label
        their_label = ffi.new("char[]", to_bytes(self.their_label))
        c_options.their_label = their_label
        c_options.favor = self.favor
        c_options.flags = self.flags
        c_options.marker_size = self.marker_size
        return c_options


class GitMergeFileInput:
    version: int
    content: str
    path: str
    mode: int

    def __init__(self, content: str, path: str, mode: int):
        self.version = 1
        self.content = content
        self.path = path
        self.mode = mode

    @classmethod
    def plain_file(cls, content: str, path: str):
        return cls(content, path, 0o100644)

    @classmethod
    def executable_file(cls, content: str, path: str):
        return cls(content, path, 0o100755)

    @classmethod
    def symlink(cls, content: str, path: str):
        return cls(content, path, 0o120000)

    @classmethod
    def binary(cls, content: str, path: str):
        return cls(content, path, 0o100644)

    @classmethod
    def from_c(cls, c_input):
        return cls(
            content=ffi.string(c_input.ptr).decode(),
            path=ffi.string(c_input.path).decode(),
            mode=c_input.mode,
        )

    def to_c(self):
        c_input = ffi.new("git_merge_file_input*")
        c_input.version = self.version
        content = ffi.new("char[]", to_bytes(self.content))
        c_input.ptr = content
        c_input.size = len(self.content)
        path = ffi.new("char[]", to_bytes(self.path))
        c_input.path = path
        c_input.mode = self.mode
        return c_input

    def __repr__(self):
        return f"<GitMergeFileInput content={self.content} path={self.path} mode={self.mode}>"

    def __str__(self):
        return self.content


class GitMergeFileResult:
    automergeable: bool
    path: str
    mode: int
    content: str

    def __init__(self, automergeable: bool, path: str, mode: int, content: str):
        self.automergeable = automergeable
        self.path = path
        self.mode = mode
        self.content = content

    @classmethod
    def from_c(cls, c_result):
        return cls(
            automergeable=bool(c_result.automergeable),
            path=ffi.string(c_result.path).decode(),
            mode=c_result.mode,
            content=ffi.string(c_result.ptr).decode(),
        )

    def __repr__(self):
        return f"<GitMergeFileResult automergeable={self.automergeable} path={self.path} mode={self.mode} content={self.content}>"

    def __str__(self):
        return self.content


def git_merge_file(
    ancestor: GitMergeFileInput,
    ours: GitMergeFileInput,
    theirs: GitMergeFileInput,
    opts: GitMergeFileOptions,
) -> GitMergeFileResult:
    c_ancestor = ancestor.to_c()
    c_ours = ours.to_c()
    c_theirs = theirs.to_c()
    c_opts = opts.to_c()
    c_result = ffi.new("git_merge_file_result *")
    error = libgit2.git_merge_file(c_result, c_ancestor, c_ours, c_theirs, c_opts)
    check_error(error)
    return GitMergeFileResult.from_c(c_result)
