import os
import pytest
from ..utils import gitservice


class TestCommitHashOfRev:
    project_path = "/home/whalien/Desktop/rocksdb"

    @classmethod
    def setup_class(cls):
        if not os.path.exists(cls.project_path):
            pytest.skip(
                f"Skipping tests because project path {cls.project_path} does not exist"
            )

    def test_two_short_hashes(self):
        assert gitservice.commit_hash_of_rev("f", self.project_path) is None

    def test_short_hash(self):
        assert (
            gitservice.commit_hash_of_rev("f9b2f0a", self.project_path)
            == "f9b2f0ad795b1041693da0857ea64d9d29705666"
        )

    def test_full_hash(self):
        assert (
            gitservice.commit_hash_of_rev(
                "f9b2f0ad795b1041693da0857ea64d9d29705666", self.project_path
            )
            == "f9b2f0ad795b1041693da0857ea64d9d29705666"
        )

    def test_tag(self):
        assert (
            gitservice.commit_hash_of_rev("2.0.fb", self.project_path)
            == "43fecffdeb68231ad0638e5373deb570ced9b41a"
        )

    def test_complex_revision_name(self):
        assert (
            gitservice.commit_hash_of_rev("2.7.fb-250-gf9b2f0ad7", self.project_path)
            == "f9b2f0ad795b1041693da0857ea64d9d29705666"
        )

    def test_illegal_hash(self):
        assert gitservice.commit_hash_of_rev("xyz123", self.project_path) is None


def test_dump_tree_objects():
    if not os.path.exists("/home/whalien/Desktop/rocksdb"):
        pytest.skip(
            "Skipping tests because project path /home/whalien/Desktop/rocksdb does not exist"
        )

    dump_tasks = [
        gitservice.DumpTask(
            dest="/tmp/rocksdb/target",
            hash="f9b2f0ad795b1041693da0857ea64d9d29705666",
            repo_path="/home/whalien/Desktop/rocksdb",
        ),
        gitservice.DumpTask(
            dest="/tmp/rocksdb/source",
            hash="5142b37000ab748433bdb5060a856663987067fb",
            repo_path="/home/whalien/Desktop/rocksdb",
        ),
        gitservice.DumpTask(
            dest="/tmp/rocksdb/base",
            hash="b2795b799e3b7ced293c95bfec1d94f5324acb0f",
            repo_path="/home/whalien/Desktop/rocksdb",
        ),
    ]

    gitservice.dump_tree_objects_par(dump_tasks)

    if not os.path.exists("/tmp/rocksdb/target"):
        pytest.fail("Dumping target tree objects failed")

    if not os.path.exists("/tmp/rocksdb/source"):
        pytest.fail("Dumping source tree objects failed")

    if not os.path.exists("/tmp/rocksdb/base"):
        pytest.fail("Dumping base tree objects failed")
