<template>
  <div class="file-tree-wrapper">
    <!-- 在el-tree外部添加一个控制开关的按钮 -->
    <el-button
      @click="toggleConflictButtons"
      v-if="!evaMode"
      class="file-op-btn"
      >{{ showConflictButtons ? "Show" : "Hide" }}&nbsp;non-conflicting
      files</el-button
    >
    <el-button @click="toggleFileButtons" v-if="!evaMode" class="file-op-btn"
      >{{ showFileButtons ? "Hide" : "Show" }}&nbsp;file operations / Shift +
      F</el-button
    >

    <el-tree
      class="file-tree-container"
      style="padding: 12px; user-select: none"
      node-key="filePath"
      :data="rootFiles"
      :props="treePropsMap"
      @node-click="handleNodeClick"
      :default-expanded-keys="[rootFile.filePath]"
      :filter-node-method="filterNode"
      empty-text="No files available"
      ref="tree"
    >
      <span slot-scope="{ data }">
        <span
          :class="{
            'conflict-file': data.isConflictFile === 1,
            'already-resolved': data.isConflictFile === 2,
            'directory-contains-conflict': data.conflictNumber !== 0,
            // 'deleted-text' : true,
          }"
        >
          <!--          rename/add -->
          <i
            :class="{
              'el-icon-circle-plus-outline': isCirclePlusModified(data),
            }"
          />
          <!--          rename/add -->
          <i
            :class="{
              'el-icon-document': data.fileType === 'file',
              'el-icon-news': data.fileType === 'direction',
            }"
          />
          {{ data.fileName }}
          <span v-if="data.conflictNumber !== 0">
            ({{ data.conflictNumber }} conflict source{{
              data.conflictNumber > 1 ? "s" : ""
            }})
          </span>
        </span>

        <!--          rename-->
        <span
          v-if="isRenameModified(data)"
          :class="{
            'conflict-file': data.isConflictFile === 1,
            'already-resolved': data.isConflictFile === 2,
            'directory-contains-conflict': data.conflictNumber !== 0,
            'deleted-text': true,
          }"
          ><i
            :class="{
              'el-icon-remove-outline': true,
            }"
          />
          {{ modifiedFile.name }}
        </span>
        <!--          rename-->
        <span @click.stop v-if="showFileButtons">
          <el-button
            v-if="data.fileName !== rootFile.fileName"
            type="text"
            size="mini"
            icon="el-icon-delete"
            title="delete"
            @click.stop="() => deleteNode(data)"
          />
          <el-button
            v-if="data.fileType === 'direction'"
            type="text"
            size="mini"
            icon="el-icon-document-add"
            title="new file"
            @click.stop="() => addNode(data, 'file')"
          />
          <el-button
            v-if="data.fileType === 'direction'"
            type="text"
            size="mini"
            icon="el-icon-folder-add"
            title="new folder"
            @click.stop="() => addNode(data, 'direction')"
          />
          <el-button
            v-if="
              data.fileName !== rootFile.fileName && data.isConflictFile !== 1
            "
            type="text"
            size="mini"
            icon="el-icon-edit"
            title="rename"
            @click.stop="() => renameNode(data)"
          />
          <el-button
            v-if="data.fileType === 'direction'"
            type="text"
            size="mini"
            icon="el-icon-upload"
            title="upload"
            @click.stop="() => uploadNode(data)"
          />
        </span>
        <!--          delete-->
        <span
          v-for="m in localModifiedFiles"
          :key="`${m.name}-${m.filePath}-${m.modifiedType}`"
        >
          <span
            v-if="
              normalizePath(data.filePath) === m.filePath &&
              m.modifiedType === 'delete'
            "
            :class="{
              'conflict-file': data.isConflictFile === 1,
              'already-resolved': data.isConflictFile === 2,
              'directory-contains-conflict': data.conflictNumber != 0,
              'deleted-text': true,
            }"
          >
            <i
              :class="{
                'el-icon-document': m.fileType === 'file',
                'el-icon-news': m.fileType === 'direction',
              }"
            />
            {{ m.name }}
          </span>
        </span>
        <!--          delete-->
      </span>
    </el-tree>
    <el-dialog
      title="File Rename"
      :visible.sync="renameDialogVisible"
      width="30%"
      @close="cancelRename"
    >
      <el-input v-model="newName" placeholder="input a new filename"></el-input>

      <span slot="footer" class="dialog-footer">
        <el-button @click="cancelRename">Cancel</el-button>
        <el-button type="primary" @click="confirmRename">Confirm</el-button>
      </span>
    </el-dialog>
    <el-dialog
      title="Delete Confirmation"
      :visible.sync="deleteDialogVisible"
      width="30%"
      @close="cancelDelete"
    >
      <p>Are you sure you want to delete this file？</p>
      <span slot="footer" class="dialog-footer">
        <el-button @click="cancelDelete">Cancel</el-button>
        <el-button type="primary" @click="confirmDelete">Confirm</el-button>
      </span>
    </el-dialog>
    <el-dialog
      title="Delete Confirmation"
      :visible.sync="deleteDirDialogVisible"
      width="30%"
      @close="cancelDelete"
    >
      <p>Are you sure you want to delete this directory？</p>
      <span slot="footer" class="dialog-footer">
        <el-button @click="cancelDelete">Cancel</el-button>
        <el-button type="primary" @click="confirmDelete">Confirm</el-button>
      </span>
    </el-dialog>

    <el-dialog
      title="Create a File"
      :visible.sync="addDialogVisible"
      width="30%"
      @close="cancelAdd"
    >
      <el-input
        v-model="newName"
        placeholder="please input the filename"
      ></el-input>
      <span slot="footer" class="dialog-footer">
        <el-button @click="cancelAdd">Cancel</el-button>
        <el-button type="primary" @click="confirmAdd">Confirm</el-button>
      </span>
    </el-dialog>
    <el-dialog
      title="Create a Directory"
      :visible.sync="addDirDialogVisible"
      width="30%"
      @close="cancelAdd"
    >
      <el-input
        v-model="newName"
        placeholder="please input the directory name"
      ></el-input>
      <span slot="footer" class="dialog-footer">
        <el-button @click="cancelAdd">Cancel</el-button>
        <el-button type="primary" @click="confirmAdd">Confirm</el-button>
      </span>
    </el-dialog>

    <el-dialog
      title="Delete Confirmation"
      :visible.sync="deleteDialogVisible"
      width="30%"
      @close="cancelDelete"
    >
      <p>Are you sure you want to delete this file?</p>
      <span slot="footer" class="dialog-footer">
        <el-button @click="cancelDelete">Cancel</el-button>
        <el-button type="primary" @click="confirmDelete">Confirm</el-button>
      </span>
    </el-dialog>
    <el-dialog
      title="Upload a File"
      :visible.sync="uploadDialogVisible"
      width="30%"
      @close="cancelUpload"
    >
      <!--        action="https://jsonplaceholder.typicode.com/posts/"-->
      <el-upload
        class="upload-demo"
        ref="upload"
        action
        :on-change="handleChange"
        :file-list="fileList"
        :auto-upload="false"
        :multiple="true"
        :limit="10"
      >
        <el-button slot="trigger" size="small" type="primary"
          >Select from local</el-button
        >
        <el-button
          style="margin-left: 10px"
          size="small"
          type="success"
          @click="submitUpload"
          :disabled="isHaveList()"
          >Upload</el-button
        >
        <div slot="tip" class="el-upload__tip">
          The size of a single file cannot exceed 5MB, and no more than 10 files
          are allowed.
        </div>
      </el-upload>

      <span slot="footer" class="dialog-footer">
        <el-button type="primary" @click="cancelUpload">End upload</el-button>
      </span>
    </el-dialog>
  </div>
</template>

<script>
const path = require("path");
import Vue from "vue";
export default {
  name: "FileTree",
  props: {
    showFileButtons: Boolean,
    showConflictButtons: Boolean,
    files: {
      type: Array,
      default: () => [],
    },
    modifiedFiles: {
      type: Array,
      default: () => [],
    },
    handleNodeClick: {
      type: Function,
      default: () => {},
    },
  },
  data() {
    return {
      treePropsMap: {
        children: "childTree",
        label: "fileName",
      },
      renameDialogVisible: false,
      addDialogVisible: false,
      addDirDialogVisible: false,
      deleteDialogVisible: false,
      deleteDirDialogVisible: false,
      uploadDialogVisible: false,
      fileList: [
        // {
        //   name: "food.jpeg",
        //   url: "https://fuss10.elemecdn.com/3/63/4e7f3a15429bfda99bce42a18cdd1jpeg.jpeg?imageMogr2/thumbnail/360x360/format/webp/quality/100",
        // },
      ],
      newName: "",
      currentNode: null,
      addType: "",
      // 添加一个作为父节点的 rootFile
      rootFile: {
        childTree: [],
        conflictNumber: 0,
        fileName: "/",
        filePath: "",
        fileType: "direction",
        isBinary: 0,
        isConflictFile: 0,
        path: "root", // 这个是什么？
      },
      modifiedFile: {
        name: "", //add->new   rename->oldname
        filePath: "", //path->newname  //delete->dir
        fileType: "",
        modifiedType: "", // unchanged,add,delete,rename(preName)
      },
      localModifiedFiles: [],
      evaMode: false,
    };
  },
  watch: {
    files: {
      handler(newFiles) {
        // 在 files 变化时更新 rootFile.childTree
        if (newFiles.length == 0) {
          return;
        }
        this.rootFile.childTree = newFiles;
        let norPath = this.rootFile.childTree[0].filePath.replace(/\\/g, "/"); // for win
        this.rootFile.filePath = path.dirname(norPath);

        // 在eva-mode下，确保树始终展开
        if (sessionStorage.getItem("eva-mode") && this.$refs.tree) {
          this.$nextTick(() => {
            this.fileTreeIfExpanded(true);
          });
        }
      },
      immediate: true, // 使得在组件创建时立即调用 handler
    },
    modifiedFiles: {
      handler(modifiedFiles) {
        this.localModifiedFiles = modifiedFiles;
        // console.log(this.localModifiedFiles);
      },
      immediate: true, // 使得在组件创建时立即调用 handler
    },
  },
  computed: {
    // 将 rootFile 与传入的 files 结合起来
    rootFiles() {
      return this.files.length ? [this.rootFile] : [];
    },
  },
  methods: {
    toggleFileButtons() {
      this.$emit("toggle-file-buttons", !this.showFileButtons);
    },
    toggleConflictButtons() {
      const newValue = !this.showConflictButtons;
      this.$emit("toggle-conflict-buttons", newValue);
      this.$refs.tree.filter(newValue);
      this.fileTreeIfExpanded(newValue);
    },
    filterNode(value, data) {
      // console.log(value, data);
      if (!value) {
        // 当 showConflictButtons 为 false 时显示所有文件
        return true;
      }
      // 当 showConflictButtons 为 true 时只显示冲突文件
      return (
        data.isConflictFile === 1 ||
        data.isConflictFile === 2 ||
        data.conflictNumber !== 0
      );
    },
    fileTreeIfExpanded(ifExpanded) {
      var nodes = this.$refs.tree.store.nodesMap;
      for (var i in nodes) {
        nodes[i].expanded = ifExpanded;
      }
    },
    deleteNode(data) {
      // 在这里触发删除操作，传递被删除节点的信息给父组件
      // this.$emit("delete-node", data);
      this.currentNode = data;
      if (data.fileType === "file") {
        this.deleteDialogVisible = true;
      } else if (data.fileType === "direction") {
        this.deleteDirDialogVisible = true;
      }
    },

    cancelDelete() {
      this.deleteDialogVisible = false;
      this.deleteDirDialogVisible = false;
    },
    confirmDelete() {
      let norPath = this.currentNode.filePath.replace(/\\/g, "/"); // for win
      const newModifiedFiles = [];
      const newModifiedFile = {
        filePath: path.dirname(norPath),
        fileType: this.currentNode.fileType,
        name: this.currentNode.fileName,
        modifiedType: "delete",
      };
      newModifiedFiles.push(newModifiedFile);
      this.$emit("delete-node", this.currentNode, newModifiedFiles);
      this.deleteDialogVisible = false;
      this.deleteDirDialogVisible = false;
      // 成功时的逻辑 .then((sucess)=>
    },

    addNode(data, addType) {
      this.currentNode = data;
      this.newName = ""; // 清空输入框
      if (addType === "file") {
        this.addDialogVisible = true;
      } else if (addType === "direction") {
        this.addDirDialogVisible = true;
      }
      this.addType = addType;
    },
    cancelAdd() {
      this.addDialogVisible = false;
      this.addDirDialogVisible = false;
    },
    confirmAdd() {
      if (this.newName.trim() === "") {
        // 提示用户输入有效的名称
        this.$message.warning("please input a valid name");
        return;
      }
      // 删除成功时的逻辑 .then((sucess)=>
      let norPath = this.currentNode.filePath.replace(/\\/g, "/"); // for win
      const newModifiedFiles = [];
      const newModifiedFile = {
        filePath: path.join(norPath, this.newName),
        fileType: this.addType,
        name: this.newName,
        modifiedType: "add",
      };
      newModifiedFiles.push(newModifiedFile);
      this.$emit(
        "add-node",
        this.currentNode,
        this.newName,
        this.addType,
        newModifiedFiles,
      );
      this.addDialogVisible = false;
      this.addDirDialogVisible = false;
    },

    uploadNode(data) {
      this.currentNode = data;
      this.fileList = [];
      this.uploadDialogVisible = true;
    },
    cancelUpload() {
      this.uploadDialogVisible = false;
    },

    //el-upload
    handleChange(file, fileList) {
      //文件数量改变
      // console.log(file);
      // console.log(fileList);
      const isLt5M = file.size / 1024 / 1024 < 5;
      if (!isLt5M) {
        this.$message({
          showClose: true,
          message: "The size of a single file cannot exceed 5MB!",
          type: "error",
        });
        // Find the index of the file in fileList
        const index = fileList.indexOf(file);

        // Remove the file from the fileList
        if (index !== -1) {
          fileList.splice(index, 1);
        }
        this.fileList = fileList;
        return false;
      }
      this.fileList = fileList;
    },
    submitUpload() {
      // // this.$refs.upload.submit();
      const newModifiedFiles = [];
      for (let i = 0; i < this.fileList.length; i++) {
        let file = this.fileList[i];
        // 删除成功时的逻辑 .then((sucess)=>
        let norPath = this.currentNode.filePath.replace(/\\/g, "/"); //dir
        const newModifiedFile = {
          filePath: path.join(norPath, file.name),
          fileType: "file",
          name: file.name,
          modifiedType: "add",
        };
        // console.log(newModifiedFile);
        // console.log(file);
        newModifiedFiles.push(newModifiedFile);
      }

      this.$emit(
        "upload-node",
        this.fileList,
        this.currentNode.filePath,
        newModifiedFiles,
      );
      this.fileList = [];
    },
    isHaveList() {
      if (this.fileList.length === 0) {
        return true;
      } else {
        return false;
      }
    },
    renameNode(data) {
      this.currentNode = data;
      this.newName = ""; // 清空输入框
      this.renameDialogVisible = true;

      // this.$emit("rename-node", data, newName);
    },
    cancelRename() {
      this.renameDialogVisible = false;
    },
    confirmRename() {
      if (this.newName.trim() !== "") {
        let norPath = this.currentNode.filePath.replace(/\\/g, "/"); // for win
        const newModifiedFiles = [];
        const newModifiedFile = {
          filePath: path.join(path.dirname(norPath), this.newName),
          fileType: this.currentNode.fileType,
          name: this.currentNode.fileName,
          modifiedType: "rename",
        };
        newModifiedFiles.push(newModifiedFile);
        // 确认重命名
        this.$emit(
          "rename-node",
          this.currentNode,
          this.newName,
          newModifiedFiles,
        );
        this.renameDialogVisible = false;
      } else {
        // 提示用户输入有效的名称
        this.$message.warning("Please input a valid name");
      }
    },
    normalizePath(path) {
      return path.replace(/\\/g, "/");
    },

    isCirclePlusModified(data) {
      for (let i = 0; i < this.modifiedFiles.length; i++) {
        let modifiedFile = this.modifiedFiles[i];
        if (modifiedFile === null) continue;
        if (
          this.normalizePath(data.filePath) === modifiedFile.filePath &&
          (modifiedFile.modifiedType === "add" ||
            modifiedFile.modifiedType === "rename")
        ) {
          return true;
        }
      }
      return false;
    },
    isRenameModified(data) {
      for (let i = 0; i < this.modifiedFiles.length; i++) {
        let modifiedFile = this.modifiedFiles[i];
        if (modifiedFile === null) continue;
        if (
          this.normalizePath(data.filePath) === modifiedFile.filePath &&
          modifiedFile.modifiedType === "rename"
        ) {
          this.modifiedFile = modifiedFile;
          return true;
        }
      }
      return false;
    },
  },
  mounted() {
    // 检查eva-mode并设置evaMode属性
    this.evaMode = !!sessionStorage.getItem("eva-mode");

    // 在eva-mode下，确保树始终展开
    // if (this.evaMode && this.$refs.tree) {
    //   this.fileTreeIfExpanded(true);
    // }
  },
};
</script>

<style scoped lang="scss">
.conflict-file {
  background-color: lightpink;
}

.already-resolved {
  background-color: lightskyblue;
}

.directory-contains-conflict {
  background-color: yellow;
}

.deleted-text {
  text-decoration: line-through;
}

.file-tree-wrapper {
  .file-op-btn {
    margin-bottom: 0.5rem;
  }

  .file-tree-container {
    border-radius: 4px;
  }
}

.el-upload__tip {
  max-width: 320px;
  word-break: keep-all;
  color: #888;
  margin: 16px auto;
}

/*.rename-container {*/
/*  position: absolute;*/
/*  right: 15px;*/
/*  padding: 5px; !* Add padding as needed *!*/
/*}*/
</style>
