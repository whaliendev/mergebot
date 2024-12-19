<template>
  <el-container>
    <el-aside width="300px">
      <div>Repo directory</div>
      <el-input
        v-model="filePath"
        placeholder="Please input directory name"
        @change="getBranchTriggerByInput"
      ></el-input>
      <div>Target Revision</div>
      <el-select
        v-model="targetBranch"
        placeholder="Please select a revision"
        :allow-create="true"
        :filterable="true"
      >
        <el-option v-for="item in branch" :key="item" :value="item">
        </el-option>
      </el-select>
      <div>Source Revision</div>
      <el-select
        v-model="sourceBranch"
        placeholder="Please select a revision"
        :allow-create="true"
        :filterable="true"
      >
        <el-option v-for="item in branch" :key="item" :value="item">
        </el-option>
      </el-select>
      <!-- <div title="Note: this is the file path on the server">
        Path of compile_commands.json
      </div>
      <el-input
        v-model="comp_db_path"
        placeholder="The compile_commands.json path"
        @blur="checkFileExistence"
        @input="clearWaringFlag"
      ></el-input>
      <div
        v-if="showCompDBWarining"
        class="text-left text-sm text-red-400 ml-4 mt-[-6px] mb-1 font-bold"
      >
        The file does not exist on the server
      </div> -->
      <el-button @click="mergeClicked" id="merge-button">Merge</el-button>
      <el-input
        type="textarea"
        :rows="6"
        placeholder="commit messge"
        v-model="textarea"
      >
      </el-input>
      <el-button @click="commitClicked"> Submit </el-button>
      <!-- <div class="link-container" v-if="showLink">
        <el-link type="primary" :href="gerritHref" target="_blank"
          >gerrit链接</el-link
        >
      </div> -->
    </el-aside>

    <el-main>
      <el-dialog
        title="Hint"
        :visible.sync="centerDialogVisible"
        width="30%"
        center
      >
        <span
          >This is a binary file, please directly select the required
          version.</span
        >
        <span slot="footer" class="dialog-footer">
          <el-button type="primary" @click="handleBinaryChoice(1)"
            >Target Branch</el-button
          >
          <el-button type="primary" @click="handleBinaryChoice(2)">
            Source Branch
          </el-button>
        </span>
      </el-dialog>

      <el-dialog
        title="Hint"
        :visible.sync="centerDialogVisibleReset"
        width="30%"
        center
      >
        <span>Do you want to reset this file?</span>
        <span slot="footer" class="dialog-footer">
          <el-button type="primary" @click="handleChoiceReset(1)"
            >Yes</el-button
          >
          <el-button type="primary" @click="handleChoiceReset(2)">No</el-button>
        </span>
      </el-dialog>

      <div v-loading="loading">
        <h3 style="margin-bottom: 0; margin-top: 0">File Tree</h3>
        <div style="margin-top: 10px; margin-bottom: 10px">
          Number of unresolved conflict files：{{ conflictNumber }}
        </div>

        <FileTree
          :files="files"
          :modifiedFiles="modifiedFiles"
          :handleNodeClick="this.handleNodeClick"
          :showFileButtons="showFileButtons"
          :showConflictButtons="showConflictButtons"
          @toggle-file-buttons="updateFileButtonsStatus"
          @toggle-conflict-buttons="updateConflictButtonsStatus"
          @delete-node="deleteTreeNode"
          @add-node="addTreeNode"
          @rename-node="renameTreeNode"
          @upload-node="uploadTreeNode"
        />
      </div>
    </el-main>
  </el-container>
</template>

<script>
import qs from "qs";
import { getSourceLanguage } from "@/utils/merge";
import {
  BlockResolutionView,
  DiffResolutionView,
  DiffResolvedView,
  PureEditorView,
} from "@/router";
import {
  addFileReq,
  deleteFileReq,
  renameFileReq,
  uploadFileReq,
} from "@/api/files";
import git from "@/api/git";
import path from "path";
import { SA_BASE_URL } from "@/api/config";

const recursiveAdd = (childTree, conflictFiles) => {
  for (const file of childTree) {
    if (file.fileType === "file") {
      if (file.isConflictFile === 1) {
        conflictFiles.push(file.filePath);
      }
    } else if (file.fileType === "direction") {
      recursiveAdd(file.childTree, conflictFiles);
    }
  }
};

export default {
  name: "FileView",
  props: ["repoRoute", "targetRoute", "sourceRoute"],
  components: {
    FileTree: () => import("@/components/FileTree.vue"),
  },

  data() {
    return {
      filePath: "", // 仓库目录
      sourceBranch: "",
      targetBranch: "",
      comp_db_path: "",
      showCompDBWarining: false,
      textarea: "",
      files: [], // 文件树
      conflictFiles: [], // for mergebot-sa ms api
      modifiedFiles: [], //文件修改树
      showFileButtons: true,
      showConflictButtons: false,
      branch: [], // 仓库分支名称
      conflictNumber: 0, // 冲突数
      repo: "",
      loading: false,

      centerDialogVisible: false,
      centerDialogVisibleReset: false,
      binaryPath: "",
      resetPath: "",

      gerritHref:
        "http://gerrit.scm.adc.com:8080/#/q/topic:80349759_202401181004",
      showLink: false,
    };
  },

  created() {
    this.filePath = sessionStorage.getItem("filePath");
    this.targetBranch = sessionStorage.getItem("targetBranch");
    this.sourceBranch = sessionStorage.getItem("sourceBranch");
    this.showLink = sessionStorage.getItem("showLink") || false;
    if (this.showLink) {
      this.gerritHref =
        "http://gerrit.scm.adc.com:8080/#/q/topic:" +
        sessionStorage.getItem("topic");
    }
    if (this.filePath) {
      this.filePath = this.filePath.replace(/\\/g, "/");
      this.getBranch();
      this.helper();
    }
  },

  methods: {
    async mergeConflict() {
      const res = await this.$axios.put(
        "/git/merge?path=" +
          this.filePath +
          "&target=" +
          this.targetBranch +
          "&source=" +
          this.sourceBranch +
          "&compdb=" +
          this.comp_db_path,
      );
      if (res.data.isSuccessful) {
        this.$message({
          showClose: true,
          message: "Merge Successfully",
          type: "success",
        });
      } else {
        throw new Error(res.data.msg);
      }
    },
    async startMergeAlgorithm() {
      const msPayload = {
        path: this.filePath,
        ms: {
          ours: this.targetBranch,
          theirs: this.sourceBranch,
        },
      };

      if (this.conflictFiles.length === 0) {
        return;
      }

      msPayload.files = this.conflictFiles;

      if (this.comp_db_path && !this.showCompDBWarining) {
        msPayload.compile_db_path = this.comp_db_path;
      }

      try {
        await this.$axios.post(`${SA_BASE_URL}/ms`, msPayload);
      } catch (err) {
        let errorMsg = "Unknown reason";
        if (err.response === null || err.response === undefined) {
          errorMsg = err.message;
        } else {
          errorMsg = err.response.data.msg;
        }

        this.$message({
          showClose: true,
          message: `Request to the API (/api/sa/ms) failed: ${errorMsg}`,
          type: "warning",
        });
      }
    },
    async mergeClicked() {
      this.loading = true;
      try {
        const mergeRoutine = async () => {
          await this.mergeConflict();
          await this.collectConflict();
          await this.helper();
        };
        // await Promise.all([mergeRoutine(), this.startMergeAlgorithm()]);
        await mergeRoutine();
        await this.startMergeAlgorithm(); // run merge algorithm after conflictFiles constructed
      } catch (err) {
        this.$message({
          showClose: true,
          message: `Merge encountered an issue: ${err}`,
          type: "error",
        });
      } finally {
        this.loading = false;
      }
    },
    async helper() {
      await this.getConflictNumber();
      await this.getTree();
    },
    commitClicked() {
      this.loading = true;

      function getGerritTopic(path) {
        const pattern = /\/mnt\/mergebot\/gerrit\/([^/]+)/;
        const matcher = path.match(pattern);
        let employeeNumber;
        if (matcher !== null) {
          // 获取匹配到的第一个组（即括号内的内容）
          employeeNumber = matcher[1];
        } else {
          // throw new Error("Invalid format for employeeNumber");
          return "";
        }
        const now = new Date();
        const year = now.getFullYear();
        const month = ("0" + (now.getMonth() + 1)).slice(-2);
        const day = ("0" + now.getDate()).slice(-2);
        const hours = ("0" + now.getHours()).slice(-2);
        const minutes = ("0" + now.getMinutes()).slice(-2);
        const formattedTime = year + month + day + hours + minutes;
        const topic = employeeNumber + "_" + formattedTime;
        return topic;
      }

      if (this.conflictNumber === 0) {
        //快速显示出gerrit提交链接（此处的提交链接无需等代码真正推送成功之后才显示在界面，执行提交时就显示在界面即可），不用管代码推送是否成功
        //一笔Gerrit提交（或变更）通常对应一个话题，而这个话题下可能存在多次push。换句话说，一个变更一般会被多次push到远程仓库中，每次push都会更新这个变更，添加新的提交或更新已有的提交。
        // let topic = "";
        // if (!this.showLink) {
        //   //第一次
        //   // topic = getGerritTopic(
        //   //   "/mnt/mergebot/gerrit/s1234sdf56/45645/你好ASD/fghfgh/sdfgdfk",
        //   // );
        //   topic = getGerritTopic(this.filePath);
        //   if (topic === "") {
        //     this.$message({
        //       showClose: true,
        //       message: "仓库格式不符合规定",
        //       type: "warning",
        //     });
        //   } else {
        //     this.showLink = true;
        //     this.gerritHref =
        //       "http://gerrit.scm.adc.com:8080/#/q/topic:" + topic;
        //     sessionStorage.setItem("showLink", true);
        //     sessionStorage.setItem("topic", topic);
        //   }
        // } else {
        //   topic = sessionStorage.getItem("topic");
        // }
        this.$axios
          .put(
            "/git/commit?path=" +
              this.filePath +
              "&message=" +
              encodeURIComponent(this.textarea) +
              "&target=" +
              this.targetBranch +
              "&topic=",
          )
          .then(response => {
            const data = response.data;
            if (data.isSuccessful) {
              if (data.repositoryState === "SAFE") {
                this.$message({
                  showClose: true,
                  message: "Commit successfully (amend, modify commit message)",
                  type: "success",
                });
              } else {
                this.$message({
                  showClose: true,
                  message: "Commit successfully",
                  type: "success",
                });
              }
            } else {
              this.$message({
                showClose: true,
                message: data.msg,
                type: "error",
              });
            }
          });
      } else {
        this.$message({
          showClose: true,
          message: "Commit failed, there are unresolved conflict files.",
          type: "warning",
        });
      }
      this.loading = false;
    },
    async collectConflict() {
      const res = await this.$axios.put(
        "/conflict/collect?path=" +
          this.filePath +
          "&target=" +
          this.targetBranch +
          "&source=" +
          this.sourceBranch,
      );
      if (res.data.code !== 200) {
        this.$message({
          showClose: true,
          message: res.data.msg,
          type: "error",
        });
      }
    },
    async handleBinaryChoice(choice) {
      // 二进制文件直接选择版本
      this.centerDialogVisible = false;
      await this.$axios
        .put(
          "files/binary?fileName=" +
            this.binaryPath +
            "&branch=" +
            choice +
            "&path=" +
            this.repo,
        )
        .then(async resp => {
          if (resp && resp.status === 200) {
            if (resp.data.code == 200) {
              this.$message({
                type: "success",
                message: "Select successfully",
              });
            } else if (resp.data.code == 500) {
              this.$message({
                type: "warning",
                message: "Select failed",
              });
            }
          }
        });

      await this.helper();
    },
    async handleChoiceReset(choice) {
      // 重置文件
      if (choice == 1) {
        this.centerDialogVisibleReset = false;
        await this.$axios
          .put(
            "/git/reset",
            qs.stringify({
              repoPath: this.repo,
              filePath: this.resetPath,
            }),
          )
          .then(resp => {
            if (resp && resp.status === 200) {
              if (resp.data.code == 200) {
                this.$message({
                  showClose: true,
                  message: "Reset successfully",
                  type: "success",
                });
              } else if (resp.data.code == 500) {
                this.$message({
                  showClose: true,
                  message: "Reset failed",
                  type: "warning",
                });
              }
            }
          });
      } else {
        this.$message({
          showClose: true,
          message: "Cancel reset",
          type: "success",
        });
      }
      this.centerDialogVisibleReset = false;
      await this.helper();
    },
    async handleNodeClick(data) {
      if (data.fileType !== "file") return;

      sessionStorage.setItem("filePath", this.filePath);
      sessionStorage.setItem("sourceBranch", this.sourceBranch);
      sessionStorage.setItem("targetBranch", this.targetBranch);
      /// isConflictFile:
      /// - 0, no merge conflicts
      /// - 1, has merge conflicts
      /// - 2, merge conflicts have been resolved
      if (data.isConflictFile == 1) {
        if (data.isBinary === 0) {
          // conflicting text file
          let normalizedPath = data.filePath.replace(/\\/g, "/");

          const routeParam = {
            fileName: data.fileName,
            filePath: normalizedPath,
            repo: this.repo,
            target: this.targetBranch,
            source: this.sourceBranch,
          };

          const fileParts = data.fileName.split(".");
          const fileExt = fileParts.length
            ? fileParts[fileParts.length - 1]
            : "java";
          const lang = getSourceLanguage(fileExt);

          // a better approach is to intercept the route and redirect to the corresponding component
          // 传入 Conflict 组件需要的参数
          // this.$router.push({
          //   name: "BlockResolution",
          //   params: routeParam,
          // });
          this.$router.push({
            name: DiffResolutionView,
            query: this.$route.query,
            params: routeParam,
          });

          // if (lang === "cpp") {
          //   this.$router.push({
          //     name: DiffResolutionView,
          //     params: routeParam,
          //   });
          // } else {
          //   // 传入 Conflict 组件需要的参数
          //   this.$router.push({
          //     name: BlockResolutionView,
          //     params: routeParam,
          //   });
          // }
        } else if (data.isBinary == 1) {
          this.binaryPath = data.path;
          this.centerDialogVisible = true;
        }
      } else if (data.isConflictFile == 2) {
        if (data.isBinary == 1) {
          this.binaryPath = data.path;
          this.centerDialogVisible = true;
        } else {
          this.resetPath = data.path;
          // console.log(this.resetPath);
          // this.centerDialogVisibleReset = true;

          // merge conflicts have been resolved
          let normalizedPath = data.filePath.replace(/\\/g, "/");

          const routeParam = {
            fileName: data.fileName,
            filePath: normalizedPath,
            repo: this.repo,
            target: this.targetBranch,
            source: this.sourceBranch,
          };

          this.$router.push({
            name: DiffResolvedView,
            query: this.$route.query,
            params: routeParam,
          });
        }
      } else {
        // 无冲突文件
        this.$router.push({
          name: PureEditorView,
          params: {
            fileName: data.fileName,
            filePath: data.filePath.replace(/\\/g, "/"),
            repo: this.repo,
          },
        });
      }
    },
    async getTree() {
      // 获取文件树
      const res = await this.$axios.get("/files?path=" + this.filePath);
      if (res.data.code === 200) {
        this.files = res.data.data;

        for (const file of this.files) {
          if (file.fileType === "file") {
            if (file.isConflictFile === 1) {
              this.conflictFiles.push(file.filePath);
            }
          } else if (file.fileType === "direction") {
            // direction? dizzy
            recursiveAdd(file.childTree, this.conflictFiles);
          }
        }
        this.conflictFiles = this.conflictFiles.reduce((acc, cur) => {
          if (!acc.includes(cur)) {
            acc.push(cur);
          }
          return acc;
        }, []); // remove duplicates
      } else {
        this.$message({
          showClose: true,
          message: res.data.msg,
          type: "error",
        });
      }
    },
    toggleFileButtons() {
      this.showFileButtons = !this.showFileButtons;
    },
    updateFileButtonsStatus(newStatus) {
      this.showFileButtons = newStatus;
    },
    updateConflictButtonsStatus(newStatus) {
      this.showConflictButtons = newStatus;
    },
    async modifyTreeNrefresh(func, modifiedFiles, ...args) {
      // 运行修改文件树的函数，然后刷新文件树
      try {
        await func(...args);
        await this.helper();
        modifiedFiles.forEach(modifiedFile =>
          this.fitModifiedFiles(modifiedFile),
        );
      } catch (err) {
        // console.error(err);
        this.$message({
          showClose: true,
          message: err,
          type: "error",
        });
      }
    },
    async deleteTreeNode(deleteNode, modifiedFiles) {
      const filePath = deleteNode.filePath;
      await this.modifyTreeNrefresh(
        deleteFileReq,
        modifiedFiles,
        filePath,
        this.repo,
      );
    },

    async addTreeNode(addNode, name, fileType, modifiedFiles) {
      // 增加文件树上的文件或目录
      const filePath = path.join(addNode.filePath, name);
      await this.modifyTreeNrefresh(
        addFileReq,
        modifiedFiles,
        filePath,
        fileType,
      );
    },

    async renameTreeNode(renameNode, newName, modifiedFiles) {
      // 修改文件树上的文件或者目录的名字
      const filePath = renameNode.filePath;
      await this.modifyTreeNrefresh(
        renameFileReq,
        modifiedFiles,
        filePath,
        newName,
        this.repo,
      );
    },

    async uploadTreeNode(fileList, dirPath, modifiedFiles) {
      // 上传文件树上的文件
      await this.modifyTreeNrefresh(
        uploadFileReq,
        modifiedFiles,
        fileList,
        dirPath,
      );
    },

    fitModifiedFiles(modifiedFile) {
      let modifiedPath = "";
      if (modifiedFile.modifiedType === "add") {
        //可能增加过去删除的同path ；                             同名问题应该创建时考虑
      } else if (modifiedFile.modifiedType === "rename") {
        //可能增加过去删除的同path  ；  原来的add，rename应该删去
        modifiedPath = path.join(
          path.dirname(modifiedFile.filePath),
          modifiedFile.name,
        );
      } else if (modifiedFile.modifiedType === "delete") {
        //                         原来的add，rename应该删去
        modifiedPath = path.join(modifiedFile.filePath, modifiedFile.name);
      }
      if (modifiedPath !== "") {
        for (let i = 0; i < this.modifiedFiles.length; i++) {
          let preModifiedFile = this.modifiedFiles[i];
          if (preModifiedFile.filePath === modifiedPath) {
            this.modifiedFiles.splice(i, 1);
            break;
          }
        }
      }
      this.modifiedFiles.push(modifiedFile);
    },

    async getBranchTriggerByInput() {
      this.targetBranch = "";
      this.sourceBranch = "";
      await this.getBranch();
      this.files = [];
      this.conflictNumber = 0;
      this.gerritHref = "";
      this.showLink = false;
      this.textarea = "";
      this.clearWaringFlag();
      this.comp_db_path = "";
    },

    async getBranch() {
      // 获取仓库分支名 List
      this.filePath = this.filePath.replace(/\\/g, "/"); // 所有 反斜杠 替换为 斜杠
      this.repo = this.filePath;
      // ! bug: uncaught AxiosError when backend down
      // ! 所有请求都应该提取到 api 文件夹下，同时调用包裹 try catch
      const res = await this.$axios.get("/branch?path=" + this.filePath);
      if (res.data.code === 200) {
        this.branch = res.data.data;
        this.$message({
          message: "Get Revisions of the repo successfully",
          type: "success",
          showClose: true,
        });
      } else {
        this.$message({
          showClose: true,
          message: res.data.msg,
          type: "error",
        });
      }
    },
    async getConflictNumber() {
      // 获取冲突文件数量
      const res = await this.$axios.get(
        "/conflict/number?repo=" + this.filePath,
      );
      if (res.data.code === 200) {
        this.conflictNumber = res.data.data;
      } else {
        this.$message({
          showClose: true,
          message: res.data.msg,
          type: "error",
        });
      }
    },
    clearWaringFlag() {
      this.showCompDBWarining = false;
    },
    async checkFileExistence() {
      if (!this.comp_db_path) {
        return;
      }
      // console.log(this.comp_db_path);
      const existence = await git.checkFileExistence({
        path: this.comp_db_path,
      });
      if (!existence) {
        this.$message({
          type: "warning",
          message: `${this.comp_db_path} 不是服务器上的一个文件`,
        });
        this.showCompDBWarining = true;
      }
    },
  },
  mounted() {
    // 使用动态路由参数初始化 data 中的属性
    if (
      this.$route.query.repo &&
      this.$route.query.source &&
      this.$route.query.target
    ) {
      // 如果存在查询参数，则执行相应的逻辑
      this.filePath = this.$route.query.repo;
      this.targetBranch = "";
      this.sourceBranch = "";
      this.getBranch();
      this.files = [];
      this.conflictNumber = 0;
      this.sourceBranch = this.$route.query.source;
      this.targetBranch = this.$route.query.target;
      this.comp_db_path = this.$route.query.compdb;
      this.checkFileExistence();
      this.mergeClicked();
    }

    window.addEventListener("keydown", event => {
      if (event.key === "F" && event.shiftKey) {
        event.preventDefault();
        this.toggleFileButtons();
      }
    });
  },
};
</script>

<style scoped>
.el-input,
.el-select,
.el-textarea {
  width: 90%;
  margin: 10px 0;
}

.el-aside {
  background-color: #d3dce6;
  color: #333;
  text-align: center;
  padding: 20px 8px;
  overflow: auto;
}

.el-main {
  background-color: #e9eef3;
  color: #333;
  text-align: center;
  overflow: auto;
}

.el-container {
  height: 100%;
}

.link-container {
  margin-top: 20px; /* 设置上边距来调整链接和按钮的间距 */
}

#merge-button {
  margin-top: 8px;
  margin-bottom: 16px;
}
</style>
