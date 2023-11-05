## mergebot-sa 接口文档

### 1 基础

#### 1.1 Base Url

mergebot-sa 部分接口有共同的基地址（base url）。格式如下：

```shell
http(s)://{host ip}:18080/api/sa
```

比如，如果部署在公网的ip`8.137.57.55`下，则ms接口的地址是：

```shell
http://8.137.57.55:18080/api/sa/ms
```

resolve接口的地址是：

```shell
http://8.137.57.55:18080/api/sa/resolve
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

对于每一个请求传输必要的字段，必要的字段的存在性，有效性均会得到检查。比如对于上述添加项目配置接口的请求体，会校验项目路径是否存在在宿主机上且是否为有效的
git 仓库。

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

其中 code 字段表示请求处理状态，如用**"00000"**表示处理成功**（非"00000"时， http
status code非200，并且一定有错误，`msg`会被填充）**；使用"U"
开头的状态表示用户操作错误，使用"C"开头的状态表示客户端的错误，使用"S"
开头的状态码表示服务端错误。此外，http
状态码也会得到对应的设置，如请求成功返回`200`，用户端和客户端错误返回`4xx`
，服务端错误返回`5xx`。

对应的，如果`code`为`00000`，`msg`会置空，`data`字段会填充对应的数据；如果`code`
字段非'00000',则`msg`字段会提示对应的错误信息，`data`字段一般为`null`。(*
*也就是说，这里的msg是error msg，只有在有错误时才会填充**)

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

mergebot-sa 目前有三个接口：更改全局配置信息、启动合并冲突解决算法、获取合并冲突解决方案。

#### 2.1 更改全局配置（**本接口未完成，在算法细节全部确定后才会提供**）

**接口功能**：本接口用于修改算法的全局配置信息。

**接口请求地址：**`{baseUrl}/config`

**请求头：**

| header key   | header value     | 说明      |
|--------------|------------------|---------|
| Content-Type | application/json | 请求体参数类型 |

**请求方法：**`POST`

**请求参数：**

| 字段              | 类型                                                                                   | 说明                                                                                      |
|-----------------|--------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------|
| styleEnabled    | bool，默认为 true                                                                        | 表示是否要启用 style based resolution                                                          |
| styleAcceptSide | enum，取值为"our", "their", "Google", "LLVM", "Mozilla", "Chromium", "WebKit"，默认为"their" | 表示当冲突为格式冲突时选用的解决方案。枚举值分别表示接受 our, 接受 their。后续五个枚举值分别表示对冲突代码重新格式化成指定的 code standard 的风格。 |
| ASTEnabled      | bool，默认为 true                                                                        | 表示是否要启用 AST based resolution                                                            |
| SimThresh       | double，取值为 0-1，默认为 0.8                                                               | 设置图匹配算法的相似度阈值                                                                           |
| CompDBLocation  | string, 默认为`./build/compile_commands.json`                                           | 本项目中的相对路径，表示 CompDB 文件在本项目中的具体位置                                                        |
| ...             | ...                                                                                  | ...                                                                                     |

以上选项随着项目的演进会逐步增加，所有的配置在项目初始化阶段会给出一份比较合理的默认参数。能够影响合并精度的参数都会给出对应的选项进行控制。项目初始化后的
post 请求会改变设置字段的值，未设置字段的值依然为默认值。

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
  "code": "C1001",
  "msg": "styleAcceptSide配置项只接受our, their, Google, LLVM, Mozilla, Chromium, WebKit为可选项的枚举值",
  "data": null
}
```

#### 2.2 启动合并冲突解决算法

**接口功能**：启动合并冲突解决程序分析算法

**说明：**由于 mergebot-sa 算法最终版的平均运行时间未知，又 http 有 timeout
时间，目前此接口在校验完参数的合理性后会直接返回成功。算法以 detach 模式运行在服务器上。通过轮询
2.3 获取合并冲突解决方案接口或建立长连接的方式拿到最后的合并冲突解决结果。

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

#### 2.3 获取合并冲突解决方案

**接口功能**：获取合并冲突解决方案

**说明：**此接口**后续可能会有变动**。根据算法最后达到的功能，响应值可能会有变动。

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

<font color="red">v1.3变更：resolve api会在"data"
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
    "projectPath": "/home/whalien/Desktop/rocksdb",
    "file": "util/env.cc",
    "resolutions": [
      {
        "code": [
          "EnvOptions::EnvOptions(const DBOptions& options) {",
          "",
          "EnvOptions EnvOptions::AdaptForLogWrite() const {",
          "  EnvOptions adapted = *this;",
          "  adapted.use_mmap_writes = false;",
          "  return adapted;",
          "}",
          "",
          "EnvOptions::EnvOptions(const Options& options) {"
        ],
        "label": "",
        "index": 0,
        "confidence": 0.7,
        "desc": "新增功能，列表合并"
      }
    ],
    "patches": [
      {
        "content": [],
        "offset": 5,
        "start": 240
      },
      {
        "start": 251,
        "offset": 2,
        "content": [
          "EnvOptions::EnvOptions(const DBOptions& options) {"
        ]
      }
    ]
  }
}
```

**注意：响应中有个`pending`字段，为`true`
时表示算法依然在处理，也就是需要加一个定时器来轮询结果。当pending为`false`
时，表示算法已处理完，即使resolutions列表为空，算法也已处理结束。**（一般在ms
endpoint调用完15s后resolve api会结束所有分析）

一个更详细的成功示例：

```json
{
  "code": "00000",
  "msg": "",
  "data": {
    "pending": false,
    "projectPath": "/home/whalien/Desktop/frameworks_av",
    "file": "services/oboeservice/AAudioServiceStreamBase.h",
    "resolutions": [
      {
        "code": [
          "    // We log the CLOSE from the close() method. We needed this separate method to log the OPEN",
          "    // because we had to wait until we generated the handle.",
          "    void logOpen(aaudio_handle_t streamHandle);",
          "",
          "    aaudio_result_t close();"
        ],
        "label": "",
        "index": 0,
        "confidence": 0.7,
        "desc": "集合包含，接受our side"
      }
    ],
    "patches": [
      {
        "start": 50,
        "offset": 7,
        "content": [
          " * It uses a subclass of AAudioServiceEndpoint to communicate with the",
          " * underlying device or port.",
          " */",
          "class AAudioServiceStreamBase : public virtual android::RefBase,",
          "                                public AAudioStreamParameters,",
          "                                public Runnable {"
        ]
      },
      {
        "content": [
          "  // We log the CLOSE from the close() method. We needed this separate method to",
          "  // log the OPEN because we had to wait until we generated the handle.",
          "  void logOpen(aaudio_handle_t streamHandle);",
          "",
          "  aaudio_result_t close();"
        ],
        "offset": 11,
        "start": 76
      },
      {
        "start": 118,
        "offset": 1,
        "content": []
      },
      {
        "start": 123,
        "offset": 17,
        "content": [
          "                                      audio_port_handle_t *clientHandle",
          "                                          __unused) {",
          "    ALOGD(",
          "        \"AAudioServiceStreamBase::startClient(%p, ...) \"",
          "        \"AAUDIO_ERROR_UNAVAILABLE\",",
          "        &client);",
          "    return AAUDIO_ERROR_UNAVAILABLE;",
          "  }",
          "",
          "  virtual aaudio_result_t stopClient(",
          "      audio_port_handle_t clientHandle __unused) {",
          "    ALOGD(\"AAudioServiceStreamBase::stopClient(%d) AAUDIO_ERROR_UNAVAILABLE\",",
          "          clientHandle);"
        ]
      },
      {
        "content": [
          "  void setSuspended(bool suspended) { mSuspended = suspended; }",
          "",
          "  bool isSuspended() const { return mSuspended; }",
          "",
          "  /**",
          "   * Atomically increment the number of active references to the stream by",
          "   * AAudioService."
        ],
        "offset": 10,
        "start": 220
      },
      {
        "content": [
          "   * Atomically decrement the number of active references to the stream by",
          "   * AAudioService. This should only be called after",
          "   * incrementServiceReferenceCount_l()."
        ],
        "offset": 2,
        "start": 238
      },
      {
        "content": [
          "  // This is used by one thread to tell another thread to exit. So it must be",
          "  // atomic."
        ],
        "offset": 1,
        "start": 312
      },
      {
        "content": [
          "                                     // TODO rename mClientHandle to mPortHandle",
          "                                     // to be more consistent with AudioFlinger."
        ],
        "offset": 1,
        "start": 317
      },
      {
        "start": 358,
        "offset": 10,
        "content": [
          "  // This indicates that a stream that is being referenced by a binder call",
          "  // needs to closed.",
          "  std::atomic<bool> mCloseNeeded{false};",
          "",
          "  // This indicate that a running stream should not be processed because of an",
          "  // error, for example a full message queue. Note that this atomic is unrelated",
          "  // to mCloseNeeded.",
          "  std::atomic<bool> mSuspended{false};"
        ]
      }
    ]
  }
}
```

如上为目前阶段的输出，后续比较确定会添加的是对于某些可以检测到的重构的提示信息。帮助开发者理解代码的语义变更。

形式比较类似于：

| refactoring_type       | node_type | confidence | before_location | before_node              | after_location | after_node                |
|------------------------|-----------|------------|-----------------|--------------------------|----------------|---------------------------|
| change field signature | field     | 1.0        | 4-4             | std::vector<DBImpl> vec; | 10-10          | auto_vector<DBImpl>  vec; |

对于每个文件内的conflict block，如果可以检测到，会展示如上表格的json格式。

失败：

```json
{
  "code": "C1000",
  "msg": "待获取冲突消解结果的文件[./db_k/db_impl.cc]不存在于Git仓库中",
  "data": null
}
```
