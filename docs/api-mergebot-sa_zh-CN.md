## mergebot-sa 接口文档

### 1 基础

#### 1.1 Base Url

mergebot-sa 部分接口有共同的基地址（base url）。格式如下：

```shell
http(s)://{host ip}:18080/api/sa
```

比如，如果部署在内网的ip`10.128.140.191`下，则ms接口的地址是：

```shell
http://10.128.140.191:18080/api/sa/ms
```

resolve接口的地址是：

```shell
http://10.128.140.191:18080/api/sa/resolve
```

#### 1.2 请求体说明

为开发简便，mergebot-sa 部分接口全部采用`POST`方法，在请求体(Body)
中传输`Content-Type`为`application/json`格式的数据。目前所有的接口均未添加鉴权。

请求体有如下格式：

```
{
    "project": "frameworks_av",
    "path": "/home/whalien/Desktop/frameworks_av"
}
```

**对于每一个请求传输必要的字段，必要的字段的存在性，有效性均会得到检查。**比如对于上述添加项目配置接口的请求体，会校验项目路径是否存在在宿主机上且是否为有效的 git 仓库。

#### 1.3 响应体说明

mergebot-sa 部分响应体遵循如下格式：

```json
{
  "code": "00000",
  "msg": "",
  "data": null
}
```

核心为`code`, `msg`和`data`三个字段。

其中 code 字段表示请求处理状态，如用"00000"表示处理成功（**非"00000"时， http status code非200，并且一定有错误，`msg`会被填充**）；使用"U"开头的状态表示用户操作错误，使用"C"开头的状态表示客户端的错误，使用"S"开头的状态码表示服务端错误。此外，http状态码也会得到对应的设置，如请求成功返回`200`，用户端和客户端错误返回`4xx`，服务端错误返回`5xx`。

对应的，如果`code`为`00000`，`msg`会置空，`data`字段会填充对应的数据；如果`code`字段非'00000',则`msg`字段会提示对应的错误信息，`data`字段一般为`null`。(**也就是说，这里的msg是error msg，只有在有错误时才会填充**)

如下为一个成功的响应体：

```json
{
  "code": "00000",
  "msg": "",
  "data": null
}
```

以下为一个请求失败的响应体：

```json
{
  "code": "U1000",
  "msg": "在路径[/home/whalien/Desktop/frameworks_av]下的项目[frameworks_av]中无冲突的C/C++文件，MBSA暂时无法处理该项目",
  "data": null
}
```

### 2 接口文档

mergebot-sa 目前有两个接口：启动合并冲突解决算法、获取合并冲突解决方案。

#### 2.1 启动合并冲突解决算法

**接口功能**：启动合并冲突解决程序分析算法

**说明：**由于 mergebot-sa 算法最终版的平均运行时间未知，又 http 有 timeout 时间，目前此接口在校验完参数的合理性后会直接返回成功。算法以 detach 模式运行在服务器上。通过轮询 2.2 获取合并冲突解决方案接口或建立长连接的方式拿到最后的合并冲突解决结果。

**接口请求地址：**`{baseUrl}/ms`

**请求头：**

| header key   | header value     | 说明      |
|--------------|------------------|---------|
| Content-Type | application/json | 请求体参数类型 |

**请求方法：**`POST`

**请求参数：**

| 字段                                               | 类型                                                                 | 说明                                                                                    |                                                                                   |
|--------------------------------------------------|--------------------------------------------------------------------|---------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------|
| project                                          | string, 可选项                                                        | 项目名称                                                                                  |                                                                                   |
| <font color="red">*</font>path                   | string, 必选项                                                        | 表示项目在宿主机器上的绝对路径                                                                       |                                                                                   |
| <font color="red">*</font>ms                     | object, 必选项，格式为`{"ours": "v3.0~146^2~62", "theirs": "2.8.fb~148"}` | 表示合并场景，其中的 ours 和 theirs 分别表示两个 commit 结点的 revision name（可以为长哈希、唯一确定提交对象的短哈希、分支名、标签名） |                                                                                   |
| <font color="red">v1.2新增字段</font>compile_db_path | string, 可选项                                                        | 表示提高算法精确度的compile_commands.json的位置                                                    | 如果传入会校验文件的存在性。<br />如果不传入算法会自动搜索项目根目录和项目根目录的build目录下。<br />如果未找到，算法会自动跳过基于图算法的分析。 |
| files                                            | list of string, 可选项。                                               | 如果不传，则表示有sa服务自行检查项目仓库下的冲突文件；若传值，则表示该合并场景下的所有冲突文件。可以为绝对路径，也可以为相对路径。                    | Debug构建的sa服务会检查列表中的第一个文件是绝对路径还是相对路径，以及是否存在于宿主机上。如果不合法会拒绝。                         |

以上选项在服务端均会校验其存在性与有效性。

**请求示例：**

```json
{
  "path": "/home/whalien/Desktop/rocksdb",
  "ms": {
    "ours": "v3.0~146^2~62",
    "theirs": "2.8.fb~148"
  },
  "files": [
    "/home/whalien/Desktop/rocksdb/db/db_impl.cc",
    "/home/whalien/Desktop/rocksdb/db/db_impl.h",
    "/home/whalien/Desktop/rocksdb/db/transaction_log_impl.cc",
    "/home/whalien/Desktop/rocksdb/db/transaction_log_impl.h",
    "/home/whalien/Desktop/rocksdb/include/rocksdb/options.h",
    "/home/whalien/Desktop/rocksdb/util/env.cc"
  ]
}
```

**响应示例：**

成功：

```json
{
  "code": "00000",
  "msg": "",
  "data": null
}
```

失败：

```json
{
  "code": "C1000",
  "msg": "合并场景哈希值不合法或不唯一",
  "data": null
}
```

#### 2.2 获取合并冲突解决方案

**接口功能**：获取合并冲突解决方案

**接口请求地址：**`{baseUrl}/resolve`

**请求头：**

| header key   | header value     | 说明      |
|--------------|------------------|---------|
| Content-Type | application/json | 请求体参数类型 |

**请求方法：**`POST`

**请求参数：**

| 字段                             | 类型                                                                 | 说明                                                                                    |
|--------------------------------|--------------------------------------------------------------------|---------------------------------------------------------------------------------------|
| project                        | string, 可选项                                                        | 项目名称                                                                                  |
| <font color="red">*</font>path | string, 必选项                                                        | 表示项目在宿主机器上的绝对路径                                                                       |
| <font color="red">*</font>file | string, 必选项                                                        | 项目中的文件相对路径，如"db/db_impl.cc"（绝对路径也行，会得到合适的处理）                                          |
| <font color="red">*</font>ms   | object, 必选项，格式为`{"ours": "v3.0~146^2~62", "theirs": "2.8.fb~148"}` | 表示合并场景，其中的 ours 和 theirs 分别表示两个 commit 结点的 revision name（可以为长哈希、唯一确定提交对象的短哈希、分支名、标签名） |

以上选项在服务端均会校验其存在性与有效性。

<font color="red">~~v1.2新增：resolve api会在"data"
字段中多返回一个patches数组，表示更准确的，针对文件的解决方案patch。其中start,
offset, content分别表示需要把冲突文件中的从start (1-based)
开始的offset行，替换为content的内容~~</font>

<font color="red">v1.3变更：resolve api 会在 "data"
字段中返回一个merged数组，表示更准确的，针对文件的合并后的解决方案，交由前端进行diff并显示给用户。</font>

**请求示例：**

```json
{
  "path": "/home/whalien/Desktop/frameworks_av",
  "ms": {
    "ours": "b9b352687e03c02958348bd25b9c4b056ebe6d63",
    "theirs": "fa24d44e438a88657246edf2bd7c4142c04b0f7e"
  },
  "file": "services/oboeservice/AAudioService.cpp"
}
```

**响应示例：**

成功：

```json
{
    "code": "00000",
    "msg": "",
    "data": {
        "pending": false,
        "projectPath": "/home/whalien/sample-repos/rocksdb/rocksdb",
        "file": "db/db_impl.h",
        "resolutions": [],
        "merged": [
            "//  Copyright (c) 2013, Facebook, Inc.  All rights reserved.",
            "//  This source code is licensed under the BSD-style license found in the",
            "//  LICENSE file in the root directory of this source tree. An additional grant",
            "//  of patent rights can be found in the PATENTS file in the same directory.",
            "//",
            "// Copyright (c) 2011 The LevelDB Authors. All rights reserved.",
            "// Use of this source code is governed by a BSD-style license that can be",
            "// found in the LICENSE file. See the AUTHORS file for names of contributors.",
            "",
            "#pragma once",
            "",
            "#include <atomic>",
            "#include <deque>",
            "#include <set>",
            "#include <utility>",
            "#include <vector>",
            "#include <string>",
            "#include \"db/dbformat.h\"",
            "#include \"db/log_writer.h\"",
            "#include \"db/snapshot.h\"",
            "#include \"db/column_family.h\"",
            "#include \"db/version_edit.h\"",
            "#include \"memtable_list.h\"",
            "#include \"port/port.h\"",
            "#include \"rocksdb/db.h\"",
            "#include \"rocksdb/env.h\"",
            "#include \"rocksdb/memtablerep.h\"",
            "#include \"rocksdb/transaction_log.h\"",
            "#include \"util/autovector.h\"",
            "#include \"util/stats_logger.h\"",
            "#include \"util/thread_local.h\"",
            "#include \"db/internal_stats.h\"",
            "",
            "namespace rocksdb {",
            "",
            "class MemTable;",
            "class TableCache;",
            "class Version;",
            "class VersionEdit;",
            "class VersionSet;",
            "",
            "class DBImpl : public DB {",
            " public:",
            "  DBImpl(const DBOptions& options, const std::string& dbname);",
            "  virtual ~DBImpl();",
            "",
            "  // Implementations of the DB interface",
            "  using DB::Put;",
            "  virtual Status Put(const WriteOptions& options,",
            "                     ColumnFamilyHandle* column_family, const Slice& key,",
            "                     const Slice& value);",
            "  using DB::Merge;",
            "  virtual Status Merge(const WriteOptions& options,",
            "                       ColumnFamilyHandle* column_family, const Slice& key,",
            "                       const Slice& value);",
            "  using DB::Delete;",
            "  virtual Status Delete(const WriteOptions& options,",
            "                        ColumnFamilyHandle* column_family, const Slice& key);",
            "  using DB::Write;",
            "  virtual Status Write(const WriteOptions& options, WriteBatch* updates);",
            "  using DB::Get;",
            "  virtual Status Get(const ReadOptions& options,",
            "                     ColumnFamilyHandle* column_family, const Slice& key,",
            "                     std::string* value);",
            "  using DB::MultiGet;",
            "  virtual std::vector<Status> MultiGet(",
            "      const ReadOptions& options,",
            "      const std::vector<ColumnFamilyHandle*>& column_family,",
            "      const std::vector<Slice>& keys, std::vector<std::string>* values);",
            "",
            "  virtual Status CreateColumnFamily(const ColumnFamilyOptions& options,",
            "                                    const std::string& column_family,",
            "                                    ColumnFamilyHandle** handle);",
            "  virtual Status DropColumnFamily(ColumnFamilyHandle* column_family);",
            "",
            "  // Returns false if key doesn't exist in the database and true if it may.",
            "  // If value_found is not passed in as null, then return the value if found in",
            "  // memory. On return, if value was found, then value_found will be set to true",
            "  // , otherwise false.",
            "  using DB::KeyMayExist;",
            "  virtual bool KeyMayExist(const ReadOptions& options,",
            "                           ColumnFamilyHandle* column_family, const Slice& key,",
            "                           std::string* value, bool* value_found = nullptr);",
            "  using DB::NewIterator;",
            "  virtual Iterator* NewIterator(const ReadOptions& options,",
            "                                ColumnFamilyHandle* column_family);",
            "  virtual Status NewIterators(",
            "      const ReadOptions& options,",
            "      const std::vector<ColumnFamilyHandle*>& column_family,",
            "      std::vector<Iterator*>* iterators);",
            "  virtual const Snapshot* GetSnapshot();",
            "  virtual void ReleaseSnapshot(const Snapshot* snapshot);",
            "  using DB::GetProperty;",
            "  virtual bool GetProperty(ColumnFamilyHandle* column_family,",
            "                           const Slice& property, std::string* value);",
            "  using DB::GetApproximateSizes;",
            "  virtual void GetApproximateSizes(ColumnFamilyHandle* column_family,",
            "                                   const Range* range, int n, uint64_t* sizes);",
            "  using DB::CompactRange;",
            "  virtual Status CompactRange(ColumnFamilyHandle* column_family,",
            "                              const Slice* begin, const Slice* end,",
            "                              bool reduce_level = false, int target_level = -1);",
            "",
            "  using DB::NumberLevels;",
            "  virtual int NumberLevels(ColumnFamilyHandle* column_family);",
            "  using DB::MaxMemCompactionLevel;",
            "  virtual int MaxMemCompactionLevel(ColumnFamilyHandle* column_family);",
            "  using DB::Level0StopWriteTrigger;",
            "  virtual int Level0StopWriteTrigger(ColumnFamilyHandle* column_family);",
            "  virtual const std::string& GetName() const;",
            "  virtual Env* GetEnv() const;",
            "  using DB::GetOptions;",
            "  virtual const Options& GetOptions(ColumnFamilyHandle* column_family) const;",
            "  using DB::Flush;",
            "  virtual Status Flush(const FlushOptions& options,",
            "                       ColumnFamilyHandle* column_family);",
            "  virtual Status DisableFileDeletions();",
            "  virtual Status EnableFileDeletions(bool force);",
            "  // All the returned filenames start with \"/\"",
            "  virtual Status GetLiveFiles(std::vector<std::string>&,",
            "                              uint64_t* manifest_file_size,",
            "                              bool flush_memtable = true);",
            "  virtual Status GetSortedWalFiles(VectorLogPtr& files);",
            "  virtual SequenceNumber GetLatestSequenceNumber() const;",
            "  virtual Status GetUpdatesSince(",
            "      SequenceNumber seq_number, unique_ptr<TransactionLogIterator>* iter,",
            "      const TransactionLogIterator::ReadOptions& read_options =",
            "          TransactionLogIterator::ReadOptions());",
            "  virtual Status DeleteFile(std::string name);",
            "",
            "  virtual void GetLiveFilesMetaData(std::vector<LiveFileMetaData>* metadata);",
            "",
            "  virtual Status GetDbIdentity(std::string& identity);",
            "",
            "  Status RunManualCompaction(ColumnFamilyData* cfd, int input_level,",
            "                             int output_level, const Slice* begin,",
            "                             const Slice* end);",
            "",
            "  // Extra methods (for testing) that are not in the public DB interface",
            "  // Compact any files in the named level that overlap [*begin, *end]",
            "  Status TEST_CompactRange(int level, const Slice* begin, const Slice* end,",
            "                           ColumnFamilyHandle* column_family = nullptr);",
            "",
            "  // Force current memtable contents to be flushed.",
            "  Status TEST_FlushMemTable();",
            "",
            "  // Wait for memtable compaction",
            "  Status TEST_WaitForFlushMemTable(ColumnFamilyHandle* column_family = nullptr);",
            "",
            "  // Wait for any compaction",
            "  Status TEST_WaitForCompact();",
            "",
            "  // Return an internal iterator over the current state of the database.",
            "  // The keys of this iterator are internal keys (see format.h).",
            "  // The returned iterator should be deleted when no longer needed.",
            "  Iterator* TEST_NewInternalIterator(",
            "      ColumnFamilyHandle* column_family = nullptr);",
            "",
            "  // Return the maximum overlapping data (in bytes) at next level for any",
            "  // file at a level >= 1.",
            "  int64_t TEST_MaxNextLevelOverlappingBytes(",
            "      ColumnFamilyHandle* column_family = nullptr);",
            "",
            "  // Simulate a db crash, no elegant closing of database.",
            "  void TEST_Destroy_DBImpl();",
            "",
            "  // Return the current manifest file no.",
            "  uint64_t TEST_Current_Manifest_FileNo();",
            "",
            "  // Trigger's a background call for testing.",
            "  void TEST_PurgeObsoleteteWAL();",
            "",
            "  // get total level0 file size. Only for testing.",
            "  uint64_t TEST_GetLevel0TotalSize();",
            "",
            "  void TEST_SetDefaultTimeToCheck(",
            "      uint64_t default_interval_to_delete_obsolete_WAL) {",
            "    default_interval_to_delete_obsolete_WAL_ =",
            "        default_interval_to_delete_obsolete_WAL;",
            "  }",
            "",
            "  void TEST_GetFilesMetaData(ColumnFamilyHandle* column_family,",
            "                             std::vector<std::vector<FileMetaData>>* metadata);",
            "",
            "  static void SuperVersionUnrefHandle(void* ptr) {",
            "    DBImpl::SuperVersion* sv = static_cast<DBImpl::SuperVersion*>(ptr);",
            "    if (sv->Unref()) {",
            "      sv->db->mutex_.Lock();",
            "      sv->Cleanup();",
            "      sv->db->mutex_.Unlock();",
            "      delete sv;",
            "    }",
            "  }",
            "",
            "  // needed for CleanupIteratorState",
            "  struct DeletionState {",
            "    inline bool HaveSomethingToDelete() const {",
            "      return candidate_files.size() || sst_delete_files.size() ||",
            "             log_delete_files.size();",
            "    }",
            "",
            "    // a list of all files that we'll consider deleting",
            "    // (every once in a while this is filled up with all files",
            "    // in the DB directory)",
            "    std::vector<std::string> candidate_files;",
            "",
            "    // the list of all live sst files that cannot be deleted",
            "    std::vector<uint64_t> sst_live;",
            "",
            "    // a list of sst files that we need to delete",
            "    std::vector<FileMetaData*> sst_delete_files;",
            "",
            "    // a list of log files that we need to delete",
            "    std::vector<uint64_t> log_delete_files;",
            "",
            "    // a list of memtables to be free",
            "    autovector<MemTable*> memtables_to_free;",
            "",
            "    autovector<SuperVersion*> superversions_to_free;",
            "",
            "    SuperVersion* new_superversion;  // if nullptr no new superversion",
            "",
            "    // the current manifest_file_number, log_number and prev_log_number",
            "    // that corresponds to the set of files in 'live'.",
            "    uint64_t manifest_file_number, log_number, prev_log_number;",
            "",
            "    explicit DeletionState(bool create_superversion = false) {",
            "      manifest_file_number = 0;",
            "      log_number = 0;",
            "      prev_log_number = 0;",
            "<<<<<<< HEAD",
            "      superversion_to_free = nullptr;",
            "      new_superversion = create_superversion ? new SuperVersion() : nullptr;",
            "||||||| b2795b799",
            "      superversion_to_free = nullptr;",
            "      new_superversion = create_superversion ? new SuperVersion() : nullptr;",
            "=======",
            "      new_superversion = create_superversion ? new SuperVersion() : nullptr;",
            ">>>>>>> 5142b37000ab748433bdb5060a856663987067fb",
            "    }",
            "",
            "    ~DeletionState() {",
            "      // free pending memtables",
            "      for (auto m : memtables_to_free) {",
            "        delete m;",
            "      }",
            "      // free superversions",
            "      for (auto s : superversions_to_free) {",
            "        delete s;",
            "      }",
            "      // if new_superversion was not used, it will be non-nullptr and needs",
            "      // to be freed here",
            "      delete new_superversion;",
            "    }",
            "  };",
            "",
            "  // Returns the list of live files in 'live' and the list",
            "  // of all files in the filesystem in 'candidate_files'.",
            "  // If force == false and the last call was less than",
            "  // options_.delete_obsolete_files_period_micros microseconds ago,",
            "  // it will not fill up the deletion_state",
            "  void FindObsoleteFiles(DeletionState& deletion_state, bool force,",
            "                         bool no_full_scan = false);",
            "",
            "  // Diffs the files listed in filenames and those that do not",
            "  // belong to live files are posibly removed. Also, removes all the",
            "  // files in sst_delete_files and log_delete_files.",
            "  // It is not necessary to hold the mutex when invoking this method.",
            "  void PurgeObsoleteFiles(DeletionState& deletion_state);",
            "",
            "  ColumnFamilyHandle* DefaultColumnFamily() const;",
            "",
            " protected:",
            "  Env* const env_;",
            "  const std::string dbname_;",
            "  unique_ptr<VersionSet> versions_;",
            "  const DBOptions options_;",
            "  Iterator* NewInternalIterator(const ReadOptions&, ColumnFamilyData* cfd,",
            "                                SuperVersion* super_version);",
            "",
            " private:",
            "  friend class DB;",
            "  friend class TailingIterator;",
            "  struct CompactionState;",
            "  struct Writer;",
            "",
            "  Status NewDB();",
            "",
            "  // Recover the descriptor from persistent storage.  May do a significant",
            "  // amount of work to recover recently logged updates.  Any changes to",
            "  // be made to the descriptor are added to *edit.",
            "  Status Recover(const std::vector<ColumnFamilyDescriptor>& column_families,",
            "                 bool read_only = false, bool error_if_log_file_exist = false);",
            "",
            "  void MaybeIgnoreError(Status* s) const;",
            "",
            "  const Status CreateArchivalDirectory();",
            "",
            "  // Delete any unneeded files and stale in-memory entries.",
            "  void DeleteObsoleteFiles();",
            "",
            "  // Flush the in-memory write buffer to storage.  Switches to a new",
            "  // log-file/memtable and writes a new descriptor iff successful.",
            "  Status FlushMemTableToOutputFile(ColumnFamilyData* cfd, bool* madeProgress,",
            "                                   DeletionState& deletion_state);",
            "",
            "  Status RecoverLogFile(uint64_t log_number, SequenceNumber* max_sequence,",
            "                        bool read_only);",
            "",
            "  // The following two methods are used to flush a memtable to",
            "  // storage. The first one is used atdatabase RecoveryTime (when the",
            "  // database is opened) and is heavyweight because it holds the mutex",
            "  // for the entire period. The second method WriteLevel0Table supports",
            "  // concurrent flush memtables to storage.",
            "  Status WriteLevel0TableForRecovery(ColumnFamilyData* cfd, MemTable* mem,",
            "                                     VersionEdit* edit);",
            "  Status WriteLevel0Table(ColumnFamilyData* cfd, autovector<MemTable*>& mems,",
            "                          VersionEdit* edit, uint64_t* filenumber);",
            "",
            "  uint64_t SlowdownAmount(int n, double bottom, double top);",
            "  Status MakeRoomForWrite(ColumnFamilyData* cfd,",
            "                          bool force /* flush even if there is room? */);",
            "  void BuildBatchGroup(Writer** last_writer,",
            "                       autovector<WriteBatch*>* write_batch_group);",
            "",
            "  // Force current memtable contents to be flushed.",
            "  Status FlushMemTable(ColumnFamilyData* cfd, const FlushOptions& options);",
            "",
            "  // Wait for memtable flushed",
            "  Status WaitForFlushMemTable(ColumnFamilyData* cfd);",
            "",
            "  void MaybeScheduleLogDBDeployStats();",
            "  static void BGLogDBDeployStats(void* db);",
            "  void LogDBDeployStats();",
            "",
            "  void MaybeScheduleFlushOrCompaction();",
            "  static void BGWorkCompaction(void* db);",
            "  static void BGWorkFlush(void* db);",
            "  void BackgroundCallCompaction();",
            "  void BackgroundCallFlush();",
            "  Status BackgroundCompaction(bool* madeProgress,",
            "                              DeletionState& deletion_state);",
            "  Status BackgroundFlush(bool* madeProgress, DeletionState& deletion_state);",
            "  void CleanupCompaction(CompactionState* compact, Status status);",
            "  Status DoCompactionWork(CompactionState* compact,",
            "                          DeletionState& deletion_state);",
            "",
            "  Status OpenCompactionOutputFile(CompactionState* compact);",
            "  Status FinishCompactionOutputFile(CompactionState* compact, Iterator* input);",
            "  Status InstallCompactionResults(CompactionState* compact);",
            "  void AllocateCompactionOutputFileNumbers(CompactionState* compact);",
            "  void ReleaseCompactionUnusedFileNumbers(CompactionState* compact);",
            "",
            "  void PurgeObsoleteWALFiles();",
            "",
            "  Status AppendSortedWalsOfType(const std::string& path,",
            "                                VectorLogPtr& log_files, WalFileType type);",
            "",
            "  // Requires: all_logs should be sorted with earliest log file first",
            "  // Retains all log files in all_logs which contain updates with seq no.",
            "  // Greater Than or Equal to the requested SequenceNumber.",
            "  Status RetainProbableWalFiles(VectorLogPtr& all_logs,",
            "                                const SequenceNumber target);",
            "  //  return true if",
            "  bool CheckWalFileExistsAndEmpty(const WalFileType type,",
            "                                  const uint64_t number);",
            "",
            "  Status ReadFirstRecord(const WalFileType type, const uint64_t number,",
            "                         WriteBatch* const result);",
            "",
            "  Status ReadFirstLine(const std::string& fname, WriteBatch* const batch);",
            "",
            "  void PrintStatistics();",
            "",
            "  // dump rocksdb.stats to LOG",
            "  void MaybeDumpStats();",
            "",
            "  // Return the minimum empty level that could hold the total data in the",
            "  // input level. Return the input level, if such level could not be found.",
            "  int FindMinimumEmptyLevelFitting(ColumnFamilyData* cfd, int level);",
            "",
            "  // table_cache_ provides its own synchronization",
            "  std::shared_ptr<Cache> table_cache_;",
            "",
            "  // Lock over the persistent DB state.  Non-nullptr iff successfully acquired.",
            "  FileLock* db_lock_;",
            "",
            "  // State below is protected by mutex_",
            "  port::Mutex mutex_;",
            "  port::AtomicPointer shutting_down_;",
            "  port::CondVar bg_cv_;  // Signalled when background work finishes",
            "  uint64_t logfile_number_;",
            "  unique_ptr<log::Writer> log_;",
            "",
            "  ColumnFamilyHandleImpl* default_cf_handle_;",
            "  unique_ptr<ColumnFamilyMemTablesImpl> column_family_memtables_;",
            "  std::deque<uint64_t> alive_log_files_;",
            "",
            "  // Thread's local copy of SuperVersion pointer",
            "  // This needs to be destructed after mutex_",
            "  ThreadLocalPtr* local_sv_;",
            "",
            "  std::string host_name_;",
            "",
            "  std::unique_ptr<Directory> db_directory_;",
            "",
            "  // Queue of writers.",
            "  std::deque<Writer*> writers_;",
            "  WriteBatch tmp_batch_;",
            "",
            "  SnapshotList snapshots_;",
            "",
            "  // Set of table files to protect from deletion because they are",
            "  // part of ongoing compactions.",
            "  std::set<uint64_t> pending_outputs_;",
            "",
            "  // count how many background compactions are running or have been scheduled",
            "  int bg_compaction_scheduled_;",
            "",
            "  // If non-zero, MaybeScheduleFlushOrCompaction() will only schedule manual",
            "  // compactions (if manual_compaction_ is not null). This mechanism enables",
            "  // manual compactions to wait until all other compactions are finished.",
            "  int bg_manual_only_;",
            "",
            "  // number of background memtable flush jobs, submitted to the HIGH pool",
            "  int bg_flush_scheduled_;",
            "",
            "  // Has a background stats log thread scheduled?",
            "  bool bg_logstats_scheduled_;",
            "",
            "  // Information for a manual compaction",
            "  struct ManualCompaction {",
            "    ColumnFamilyData* cfd;",
            "    int input_level;",
            "    int output_level;",
            "    bool done;",
            "    Status status;",
            "    bool in_progress;          // compaction request being processed?",
            "    const InternalKey* begin;  // nullptr means beginning of key range",
            "    const InternalKey* end;    // nullptr means end of key range",
            "    InternalKey tmp_storage;   // Used to keep track of compaction progress",
            "  };",
            "  ManualCompaction* manual_compaction_;",
            "",
            "  // Have we encountered a background error in paranoid mode?",
            "  Status bg_error_;",
            "",
            "  std::unique_ptr<StatsLogger> logger_;",
            "",
            "  int64_t volatile last_log_ts;",
            "",
            "  // shall we disable deletion of obsolete files",
            "  // if 0 the deletion is enabled.",
            "  // if non-zero, files will not be getting deleted",
            "  // This enables two different threads to call",
            "  // EnableFileDeletions() and DisableFileDeletions()",
            "  // without any synchronization",
            "  int disable_delete_obsolete_files_;",
            "",
            "  // last time when DeleteObsoleteFiles was invoked",
            "  uint64_t delete_obsolete_files_last_run_;",
            "",
            "  // last time when PurgeObsoleteWALFiles ran.",
            "  uint64_t purge_wal_files_last_run_;",
            "",
            "  // last time stats were dumped to LOG",
            "  std::atomic<uint64_t> last_stats_dump_time_microsec_;",
            "",
            "  // obsolete files will be deleted every this seconds if ttl deletion is",
            "  // enabled and archive size_limit is disabled.",
            "  uint64_t default_interval_to_delete_obsolete_WAL_;",
            "",
            "  bool flush_on_destroy_;  // Used when disableWAL is true.",
            "  static const int KEEP_LOG_FILE_NUM = 1000;",
            "  std::string db_absolute_path_;",
            "",
            "  // count of the number of contiguous delaying writes",
            "  int delayed_writes_;",
            "",
            "  // The options to access storage files",
            "  const EnvOptions storage_options_;",
            "",
            "  // A value of true temporarily disables scheduling of background work",
            "  bool bg_work_gate_closed_;",
            "",
            "  // Guard against multiple concurrent refitting",
            "  bool refitting_level_;",
            "",
            "  // Indicate DB was opened successfully",
            "  bool opened_successfully_;",
            "",
            "  // No copying allowed",
            "  DBImpl(const DBImpl&);",
            "  void operator=(const DBImpl&);",
            "",
            "  // dump the delayed_writes_ to the log file and reset counter.",
            "  void DelayLoggingAndReset();",
            "",
            "  // Return the earliest snapshot where seqno is visible.",
            "  // Store the snapshot right before that, if any, in prev_snapshot",
            "  inline SequenceNumber findEarliestVisibleSnapshot(",
            "      SequenceNumber in, std::vector<SequenceNumber>& snapshots,",
            "      SequenceNumber* prev_snapshot);",
            "",
            "  // Background threads call this function, which is just a wrapper around",
            "  // the cfd->InstallSuperVersion() function. Background threads carry",
            "  // deletion_state which can have new_superversion already allocated.",
            "  void InstallSuperVersion(ColumnFamilyData* cfd,",
            "                           DeletionState& deletion_state);",
            "",
            "  using DB::GetPropertiesOfAllTables;",
            "  virtual Status GetPropertiesOfAllTables(",
            "      ColumnFamilyHandle* column_family,",
            "      TablePropertiesCollection* props) override;",
            "",
            "  // Function that Get and KeyMayExist call with no_io true or false",
            "  // Note: 'value_found' from KeyMayExist propagates here",
            "  Status GetImpl(const ReadOptions& options, ColumnFamilyHandle* column_family,",
            "                 const Slice& key, std::string* value,",
            "                 bool* value_found = nullptr);",
            "  void ResetThreadLocalSuperVersions(DeletionState* deletion_state);",
            "",
            "  // Move the files in the input level to the target level.",
            "  // If target_level < 0, automatically calculate the minimum level that could",
            "  // hold the data set.",
            "  Status ReFitLevel(ColumnFamilyData* cfd, int level, int target_level = -1);",
            "",
            "  // Returns a pair of iterators (mutable-only and immutable-only) used",
            "  // internally by TailingIterator and stores cfd->GetSuperVersionNumber() in",
            "  // *superversion_number. These iterators are always up-to-date, i.e. can",
            "  // be used to read new data.",
            "  std::pair<Iterator*, Iterator*> GetTailingIteratorPair(",
            "      const ReadOptions& options, ColumnFamilyData* cfd,",
            "      uint64_t* superversion_number);",
            "};",
            "",
            "// Sanitize db options.  The caller should delete result.info_log if",
            "// it is not equal to src.info_log.",
            "extern Options SanitizeOptions(const std::string& db,",
            "                               const InternalKeyComparator* icmp,",
            "                               const InternalFilterPolicy* ipolicy,",
            "                               const Options& src);",
            "",
            "extern DBOptions SanitizeOptions(const std::string& db, const DBOptions& src);",
            "",
            "// Determine compression type, based on user options, level of the output",
            "// file and whether compression is disabled.",
            "// If enable_compression is false, then compression is always disabled no",
            "// matter what the values of the other two parameters are.",
            "// Otherwise, the compression type is determined based on options and level.",
            "CompressionType GetCompressionType(const Options& options, int level,",
            "                                   const bool enable_compression);",
            "",
            "// Determine compression type for L0 file written by memtable flush.",
            "CompressionType GetCompressionFlush(const Options& options);",
            "",
            "}  // namespace rocksdb"
        ]
    }
}
```

**注意：响应中有个`pending`字段，为`true`时表示算法依然在处理，也就是需要加一个定时器来轮询结果。当pending为`false`时，表示算法已处理完，即使resolutions，merged列表为空，算法也已处理结束。**（一般在ms endpoint调用完15s后resolve api会结束所有分析）

一个更详细的成功示例：

```json
{
    "code": "00000",
    "msg": "",
    "data": {
        "pending": false,
        "projectPath": "/home/whalien/sample-repos/rocksdb/rocksdb",
        "file": "db/transaction_log_impl.h",
        "resolutions": [
            {
                "code": [
                    "const DBOptions* options_;",
                    "",
                    "const Options* options_;",
                    "",
                    "const TransactionLogIterator::ReadOptions read_options_;"
                ],
                "label": "",
                "index": 1,
                "confidence": 0.7,
                "desc": "声明合并"
            }
        ],
        "merged": [
            "//  Copyright (c) 2013, Facebook, Inc.  All rights reserved.",
            "//  This source code is licensed under the BSD-style license found in the",
            "//  LICENSE file in the root directory of this source tree. An additional grant",
            "//  of patent rights can be found in the PATENTS file in the same directory.",
            "//",
            "",
            "#pragma once",
            "#include <vector>",
            "#include \"rocksdb/env.h\"",
            "#include \"rocksdb/options.h\"",
            "#include \"rocksdb/types.h\"",
            "#include \"rocksdb/transaction_log.h\"",
            "#include \"db/db_impl.h\"",
            "#include \"db/log_reader.h\"",
            "#include \"db/filename.h\"",
            "",
            "namespace rocksdb {",
            "",
            "struct LogReporter : public log::Reader::Reporter {",
            "  Env* env;",
            "  Logger* info_log;",
            "  virtual void Corruption(size_t bytes, const Status& s) {",
            "    Log(info_log, \"dropping %zu bytes; %s\", bytes, s.ToString().c_str());",
            "  }",
            "  virtual void Info(const char* s) { Log(info_log, \"%s\", s); }",
            "};",
            "",
            "class LogFileImpl : public LogFile {",
            " public:",
            "  LogFileImpl(uint64_t logNum, WalFileType logType, SequenceNumber startSeq,",
            "              uint64_t sizeBytes)",
            "      : logNumber_(logNum),",
            "        type_(logType),",
            "        startSequence_(startSeq),",
            "        sizeFileBytes_(sizeBytes) {}",
            "",
            "  std::string PathName() const {",
            "    if (type_ == kArchivedLogFile) {",
            "      return ArchivedLogFileName(\"\", logNumber_);",
            "    }",
            "    return LogFileName(\"\", logNumber_);",
            "  }",
            "",
            "  uint64_t LogNumber() const { return logNumber_; }",
            "",
            "  WalFileType Type() const { return type_; }",
            "",
            "  SequenceNumber StartSequence() const { return startSequence_; }",
            "",
            "  uint64_t SizeFileBytes() const { return sizeFileBytes_; }",
            "",
            "  bool operator<(const LogFile& that) const {",
            "    return LogNumber() < that.LogNumber();",
            "  }",
            "",
            " private:",
            "  uint64_t logNumber_;",
            "  WalFileType type_;",
            "  SequenceNumber startSequence_;",
            "  uint64_t sizeFileBytes_;",
            "};",
            "",
            "class TransactionLogIteratorImpl : public TransactionLogIterator {",
            " public:",
            "  TransactionLogIteratorImpl(const std::string& dir, const DBOptions* options,",
            "                             const EnvOptions& soptions,",
            "                             const SequenceNumber seqNum,",
            "                             std::unique_ptr<VectorLogPtr> files,",
            "                             DBImpl const* const dbimpl);",
            "",
            "  TransactionLogIteratorImpl(",
            "      const std::string& dir, const Options* options,",
            "      const TransactionLogIterator::ReadOptions& read_options,",
            "      const EnvOptions& soptions, const SequenceNumber seqNum,",
            "      std::unique_ptr<VectorLogPtr> files, DBImpl const* const dbimpl);",
            "",
            "  virtual bool Valid();",
            "",
            "  virtual void Next();",
            "",
            "  virtual Status status();",
            "",
            "  virtual BatchResult GetBatch();",
            "",
            " private:",
            "  const std::string& dir_;",
            "  const DBOptions* options_;",
            "  const TransactionLogIterator::ReadOptions read_options_;",
            "  const EnvOptions& soptions_;",
            "  SequenceNumber startingSequenceNumber_;",
            "  std::unique_ptr<VectorLogPtr> files_;",
            "  bool started_;",
            "  bool isValid_;  // not valid when it starts of.",
            "  Status currentStatus_;",
            "  size_t currentFileIndex_;",
            "  std::unique_ptr<WriteBatch> currentBatch_;",
            "  unique_ptr<log::Reader> currentLogReader_;",
            "  Status OpenLogFile(const LogFile* logFile, unique_ptr<SequentialFile>* file);",
            "  LogReporter reporter_;",
            "  SequenceNumber currentBatchSeq_;  // sequence number at start of current batch",
            "  SequenceNumber currentLastSeq_;   // last sequence in the current batch",
            "  DBImpl const* const dbimpl_;      // The db on whose log files this iterates",
            "                                    // Reads from transaction log only if the",
            "                                    // writebatch record has been written",
            "  bool RestrictedRead(Slice* record, std::string* scratch);",
            "  // Seeks to startingSequenceNumber reading from startFileIndex in files_.",
            "  // If strict is set,then must get a batch starting with startingSequenceNumber",
            "  void SeekToStartSequence(uint64_t startFileIndex = 0, bool strict = false);",
            "  // Implementation of Next. SeekToStartSequence calls it internally with",
            "  // internal=true to let it find next entry even if it has to jump gaps because",
            "  // the iterator may start off from the first available entry but promises to",
            "  // be continuous after that",
            "  void NextImpl(bool internal = false);",
            "  // Check if batch is expected, else return false",
            "  bool IsBatchExpected(const WriteBatch* batch, SequenceNumber expectedSeq);",
            "  // Update current batch if a continuous batch is found, else return false",
            "  void UpdateCurrentWriteBatch(const Slice& record);",
            "  Status OpenLogReader(const LogFile* file);",
            "};",
            "",
            "}  //  namespace rocksdb"
        ]
    }
}
```

失败：

```json
{
  "code": "C1000",
  "msg": "待获取冲突消解结果的文件[./db_k/db_impl.cc]不存在于Git仓库中",
  "data": null
}
```
