<template>
  <el-container class="diff-resolution-container">
    <el-header class="diff-resolution-header" height="56px">
      <div class="controls-container">
        <el-button
          icon="el-icon-back"
          @click="handleBackClicked"
          size="medium"
        ></el-button>
        <div class="ml-3 text-[#606266]">{{ relativePath }}</div>

        <div
          class="ml-1 status-container flex items-center space-around"
          :class="{ visible: pending, invisible: !pending }"
        >
          <el-button plain :loading="pending" size="medium"></el-button>
          算法运行中...
        </div>
      </div>
      <!-- controls-container -->

      <div class="commit-container">
        <el-button
          v-show="refactoringList && refactoringList.length"
          icon="el-icon-tickets"
          size="medium"
          plain
          @click="showRefactoringDrawer"
          >展示变更信息</el-button
        >
        <thumb-up-button
          class="ml-2 mr-2"
          ref="thumbRef"
          @like-requested="onLikeRequested"
          :liked="isLiked"
        />
        <el-button
          icon="el-icon-circle-check"
          size="medium"
          plain
          type="success"
          @click="handleStageRequest"
          >暂存</el-button
        >
        <el-button
          icon="el-icon-refresh"
          @click="handleResetClicked"
          size="medium"
          plain
          type="danger"
          >重置</el-button
        >
      </div>
      <!-- commit-container -->
    </el-header>

    <el-main class="diff-resolution-body-container">
      <draggable-container :number="2" dir="v" :config="initRowConfigs">
        <draggable-item :index="0" :show-touch-bar="false">
          <refactor-miner-editor
            v-if="showRMinerEditor"
            :rev-arr="refactorMinerRevArr"
          />
          <merge-scenario-editor
            v-else
            :rev-arr="revArr"
            :reveal-ratio="blockStartRatio"
          />
        </draggable-item>
        <draggable-item :index="1" class="min-h-[2%]">
          <diff-resolution-editor
            :lang="params.lang"
            :original="originalContent"
            :modified="modifiedContent"
            @update:modified="handleModifiedUpdate"
            @block-change="handleConflictBlockChange"
          />
        </draggable-item>
      </draggable-container>
    </el-main>

    <el-drawer
      title="变更信息"
      custom-class="refactoring-drawer"
      :visible="drawerVisible"
      size="32%"
      direction="rtl"
      :modal="false"
      @close="handleDrawerClose"
    >
      <ol class="refactoring-list">
        <li
          v-for="(item, index) in refactoringList"
          :key="index"
          @click="showRefactoringAt(item.idx)"
        >
          {{ index + 1 }}.&nbsp;&nbsp;<el-tag>{{ item.type }}</el-tag>
          <span
            title="点击以定位到代码段"
            class="ml-2 italic"
            style="font-family: monospace"
            >{{ item.description }}</span
          >
        </li>
      </ol>
    </el-drawer>
  </el-container>
</template>

<script>
import DraggableContainer from "@/components/DraggableContainer.vue";
import DraggableItem from "@/components/DraggableItem.vue";
import MergeScenarioEditor from "@/components/MergeScenarioEditor.vue";
import DiffResolutionEditor from "@/components/DiffResolutionEditor.vue";
import RefactorMinerEditor from "@/components/RefactorMinerEditor.vue";
import ThumbUpButton from "@/components/ThumbUpButton.vue";
import { getSourceLanguage } from "@/utils";
import { getSpecifiedConflictingMeta, writeResolvedToFile } from "@/api/git";
import { thumbUpFile, thumbDownFile, saveFileTrackingData } from "@/api/audit";
import { BlockResolutionView, FileTreeView } from "@/router";

export default {
  name: "DiffResolution",
  components: {
    DraggableContainer,
    DraggableItem,
    MergeScenarioEditor,
    DiffResolutionEditor,
    RefactorMinerEditor,
    ThumbUpButton,
  },
  data: function () {
    return {
      // configs, the first row: 240px, the second row: auto
      initRowConfigs: [{ default: 240 }, { default: undefined }],

      // at creation, used as params
      params: {
        fileName: "",
        filePath: "",
        lang: "",
        repo: "",
        targetBranch: "",
        sourceBranch: "",
      },

      // metadata
      meta: {
        target: "",
        base: "",
        source: "",
        conflict: "",
        merged: "",

        tempPath: "",
        targetPath: "",
        sourcePath: "",
        basePath: "",
        conflictPath: "",

        conflictBlocksInfo: [], // info of conflict blocks
      },

      // rev arr pass to merge-scenario-editor
      revArr: [
        {
          title: "target",
          content:
            'int main() {\n    printf("Hello World!");\n    return 0;\n}',
          lang: "cpp",
          disabled: true, // disable intends to disable drag bar
        },
        {
          title: "base",
          content:
            'int main() {\n    printf("Hello World!");\n    return 0;\n}',
          lang: "cpp",
          disabled: false,
        },
        {
          title: "source",
          content:
            'int main() {\n    printf("Hello World!");\n    return 0;\n}',
          lang: "cpp",
          disabled: false,
        },
      ],

      blockStartRatio: -1,

      drawerVisible: false,

      thumbRef: null,
    };
  },
  computed: {
    // check if route params are valid
    isValidRouteParams() {
      return (
        this.params.fileName &&
        this.params.filePath &&
        this.params.repo &&
        this.params.targetBranch &&
        this.params.sourceBranch
      );
    },
    isSATimedout() {
      return this.$store.state.diff.sa.timeout;
    },
    originalContent() {
      return this.$store.getters["diff/originalContent"];
    },
    modifiedContent() {
      return this.$store.getters["diff/modifiedContent"];
    },
    rtResolvedFileContent() {
      return this.$store.getters["diff/rtResolvedFileContent"];
    },
    pending() {
      // return true;
      return this.$store.state.diff.sa.pending;
    },
    relativePath() {
      if (!this.params.repo || !this.params.filePath) {
        return this.params.filePath;
      }
      if (this.params.filePath.startsWith(this.params.repo)) {
        const rel = this.params.filePath.slice(this.params.repo.length);
        if (rel.startsWith("/") || rel.startsWith("\\")) {
          return rel.slice(1);
        }
        return rel;
      } else {
        return this.params.filePath;
      }
    },
    remainingConflictBlocksCnt() {
      return this.$store.getters["diff/remainingConflictBlocksCnt"];
    },
    showRMinerEditor() {
      return this.params.lang === "java";
    },
    refactorMinerRevArr() {
      if (this.revArr.length === 3) {
        return [this.revArr[0], this.revArr[2]];
      }
      return [
        {
          title: "目标分支版本",
          content: "未获取到目标分支版本文件内容",
          lang: "java",
          disabled: true,
        },
        {
          title: "源分支版本",
          content: "未获取到源分支版本文件内容",
          lang: "java",
          disabled: false,
        },
      ];
    },

    refactoringList() {
      const refactoringList =
        this.$store.getters["refactoring/refactoringDrawerList"];
      // console.log(refactoringList);
      return refactoringList;
    },

    isLiked() {
      let likeStatus = sessionStorage.getItem(this.params.filePath);
      if (likeStatus === null) {
        sessionStorage.setItem(this.params.filePath, false);
        return false;
      } else if (likeStatus === "true") {
        return true;
      } else {
        return false;
      }
    },
  },
  watch: {
    isSATimedout(newVal) {
      if (newVal) {
        this.$message.error("获取SA解决方案超时");
      }
    },
  },
  async created() {
    this.params.fileName = this.$route.params.fileName;
    this.params.filePath = this.$route.params.filePath;
    // console.log(this.filePath, this.fileName);
    if (this.params.fileName) {
      const parts = this.params.fileName.split(".");
      if (parts.length) {
        this.params.lang = getSourceLanguage(parts[parts.length - 1]);
      }
    }
    this.params.repo = this.$route.params.repo;
    this.params.targetBranch = this.$route.params.target;
    this.params.sourceBranch = this.$route.params.source;

    if (!this.isValidRouteParams) {
      this.$message.error("路由参数无效，请返回文件树重新选择文件");
      return;
    }

    await this.getSpecifiedConflictingMeta();
    this.fillRevArr();
    this.initResolveEditorContent();

    // fix: when remainingConflictBlocksCnt is 0, the following dispatches will not be executed
    if (this.params.lang && this.remainingConflictBlocksCnt) {
      await this.dispatchGetMLResolutions();
      await this.dispatchGetDLResolutions();

      if (this.params.lang === "cpp") {
        await this.dispatchGetSAResolutions();
      }

      this.$store.commit("refactoring/clearRefactorings");
      this.$store.commit("refactoring/setShowRefactoringIdx", -1);
      if (this.params.lang === "java") {
        await this.dispatchGetRefactorings();
      }
    }
  },
  methods: {
    handleConflictBlockChange(e) {
      this.blockStartRatio = e.percentage;
      // console.log(this.blockStartRatio);
    },
    handleBackClicked() {
      this.$router.push({ name: FileTreeView, query: this.$route.query });
    },
    async handleStageRequest() {
      this.$store.commit("diff/refreshRemainingConflictBlocks");
      if (this.remainingConflictBlocksCnt) {
        this.$message.warning("还有未解决的冲突块，请解决完所有冲突块后再暂存");
        return;
      }
      const payload = {
        path: this.meta.conflictPath,
        fileName: this.params.fileName,
        content: this.rtResolvedFileContent,
        repo: this.params.repo,
        tempPath: this.meta.tempPath,
      };
      try {
        const res = await writeResolvedToFile(payload);
        if (res) {
          this.$message.success("暂存成功");
        }
        const saveres = await saveFileTrackingData({
          projectPath: this.params.repo,
          targetBranch: this.params.targetBranch,
          sourceBranch: this.params.sourceBranch,
          fileName: this.params.filePath,
          blockCnt: this.meta.conflictBlocksInfo.length,
        });
        if (saveres.code !== "200") {
          this.$message({
            showClose: true,
            message: saveres.msg,
            type: "error",
          });
        }
      } catch (error) {
        this.$message.error(error.message);
      }
    },
    handleResetClicked() {
      this.$store.commit("diff/resetResolveEditor");
    },
    handleModifiedUpdate(payload) {
      this.$store.commit("diff/saveModifiedContent", payload);
      // this.meta.merged = newVal;
      // this.diffEditorProps.modified = newVal; // this will cause infinite loop
    },
    handleSwitchViewClicked() {
      this.$router.push({
        name: BlockResolutionView,
        query: this.$route.query,
        params: {
          fileName: this.params.fileName,
          filePath: this.params.filePath,
          repo: this.params.repo,
          target: this.params.targetBranch,
          source: this.params.sourceBranch,
        },
      });
    },
    fillRevArr() {
      this.revArr.splice(0, this.revArr.length);
      this.revArr.push({
        // title: this.params.targetBranch || 'target',
        title: "目标分支版本",
        content: this.meta.target,
        lang: this.params.lang,
        disabled: true,
      });
      this.revArr.push({
        title: "基础版本",
        content: this.meta.base,
        lang: this.params.lang,
        disabled: false,
      });
      this.revArr.push({
        // title: this.params.sourceBranch || 'source',
        title: "源分支版本",
        content: this.meta.source,
        lang: this.params.lang,
        disabled: false,
      });
    },

    initResolveEditorContent() {
      this.$store.commit("diff/initResolveEditorContent", this.meta.conflict);
      let fileParams = {
        projectPath: this.params.repo,
        targetBranch: this.params.targetBranch,
        sourceBranch: this.params.sourceBranch,
        fileName: this.params.fileName,
        filePath: this.params.filePath,
      };
      this.$store.commit("diff/initFileParams", fileParams);
    },
    async getSpecifiedConflictingMeta() {
      try {
        const metadata = await getSpecifiedConflictingMeta(
          this.params.filePath,
          this.params.repo,
        );
        if (metadata) {
          // console.log(metadata);
          this.meta.target = metadata.ours || "";
          this.meta.base = metadata.base || "";
          this.meta.source = metadata.theirs || "";
          this.meta.conflict = metadata.conflict.join("\n") || "";
          this.meta.merged = this.meta.conflict;

          this.meta.tempPath = metadata.tempPath || "";
          this.meta.targetPath = metadata.targetPath || "";
          this.meta.sourcePath = metadata.sourcePath || "";
          this.meta.basePath = metadata.basePath || "";
          this.meta.conflictPath = metadata.conflictPath || "";

          this.meta.conflictBlocksInfo = metadata.info || {};

          // console.log('init block', this.meta.conflictBlocksInfo);
          // if (this.meta.conflictBlocksInfo.length) {
          //   this.$store.commit('diff/saveHistoryResolutions', this.meta.conflictBlocksInfo);
          // }
        }
      } catch (error) {
        this.$message.error(error.message);
      }
    },
    async dispatchGetDLResolutions() {
      try {
        return await this.$store.dispatch("diff/getDLResolutions", {
          blocks: this.meta.conflictBlocksInfo,
        });
      } catch (error) {
        this.$message.error(`DL: ${error.message}`);
      }
    },
    async dispatchGetMLResolutions() {
      // console.log('at dispatch', this.meta.conflictBlocksInfo);
      try {
        return await this.$store.dispatch("diff/getMLResolutions", {
          payload: {
            path: this.params.repo,
            target: this.params.targetBranch,
            source: this.params.sourceBranch,
            version1: this.meta.targetPath.replace(/\\/g, "/"),
            version2: this.meta.sourcePath.replace(/\\/g, "/"),
            conflict: this.meta.conflictPath.replace(/\\/g, "/"),
            base: this.meta.basePath.replace(/\\/g, "/"),
            filetype: this.params.lang,
          },
          blocks: this.meta.conflictBlocksInfo,
        });
      } catch (error) {
        this.$message.error(`ML: ${error.message}`);
      }
    },

    async dispatchGetSAResolutions() {
      try {
        return await this.$store.dispatch("diff/getSAResolutions", {
          path: this.params.repo,
          ms: {
            ours: this.params.targetBranch,
            theirs: this.params.sourceBranch,
          },
          file: this.params.filePath,
        });
      } catch (error) {
        this.$message.error(`SA: ${error.message}`);
      }
    },

    async dispatchGetRefactorings() {
      if (!this.meta.target || !this.meta.source || !this.params.filePath) {
        return;
      }
      try {
        return await this.$store.dispatch("refactoring/getRefactorings", {
          filePath: this.params.filePath,
          targetContent: this.meta.target,
          sourceContent: this.meta.source,
        });
      } catch (error) {
        this.$message.error(`获取变更信息异常: ${error.message}`);
      }
    },

    handleDrawerClose() {
      this.drawerVisible = false;
    },

    showRefactoringDrawer() {
      this.drawerVisible = true;
    },

    showRefactoringAt(idx) {
      this.drawerVisible = false;
      this.$store.commit("refactoring/setShowRefactoringIdx", idx);
    },

    async onLikeRequested(likeVal) {
      let fileParams = {
        projectPath: this.params.repo,
        targetBranch: this.params.targetBranch,
        sourceBranch: this.params.sourceBranch,
        fileName: this.params.filePath,
      };
      if (likeVal) {
        try {
          const res = await thumbUpFile(fileParams);
          if (res.code === "200") {
            this.$message({
              showClose: true,
              message: "点赞成功",
              type: "success",
            });
            sessionStorage.setItem(this.params.filePath, true);
          } else {
            this.$message({
              showClose: true,
              message: res.msg,
              type: "error",
            });
          }
        } catch (error) {
          this.$message.error(error.message);
          // this.thumbRef.setLikeRequestedResult(false);
        }
      } else {
        try {
          let res = await thumbDownFile(fileParams);
          if (res.code === "200") {
            this.$message({
              showClose: true,
              message: "取消点赞成功",
              type: "success",
            });
            sessionStorage.setItem(this.params.filePath, false);
          } else {
            this.$message({
              showClose: true,
              message: res.msg,
              type: "error",
            });
          }
        } catch (error) {
          this.$message.error(error.message);
          // this.thumbRef.setLikeRequestedResult(true);
        }
      }
    },
  },
};
</script>

<style scoped lang="scss">
.el-container {
  height: 100vh;
  width: 100vw;
  overflow: hidden;
}

.el-header {
  background-color: #b3c0d1;
  display: flex;
  justify-content: space-between;
  align-items: center;

  & .controls-container {
    display: flex;
    justify-content: space-between;
    align-items: center;
    font-size: 18px;
    color: #606266;

    & .status-container {
      font-size: 14px;
      font-style: italic;

      :deep(.el-button) {
        background-color: #b3c0d1;
        border: 0;
        padding-left: 0.5rem;
        padding-right: 0.5rem;

        &::before {
          background-color: #b3c0d1;
        }
      }
    }
  }
}

.progress-bar-container {
  display: inline-block;
  width: 200px;
  /* height: 40px; */

  margin-left: 8px;
}

.diff-resolution-body-container {
  padding: 0;
}

:deep(.el-progress-bar__outer) {
  border-radius: 24px;
  height: 30px !important;
}

:deep(.el-progress-bar__innerText) {
  color: #eee !important;
  font-weight: 500;
}
</style>

<style lang="scss">
.refactoring-drawer {
  .el-drawer__header {
    margin-bottom: 0.8rem;
  }

  .refactoring-list {
    li {
      margin-bottom: 0.8rem;

      &:hover {
        cursor: pointer;
        text-decoration: underline;
        text-decoration-color: #1ea366;
      }
    }
  }

  .el-drawer__body {
    padding: 1rem;
  }
}
</style>
