<template>
  <el-container>
    <el-header>
      <div class="left-area">
        <el-button icon="el-icon-back" @click="handleBackClicked"></el-button>
        <el-button-group>
          <el-button icon="el-icon-arrow-up" @click="changeConflictBlockBy(-1)"
            >上一个冲突块</el-button
          >
          <el-button>{{ index }}/{{ conflictBlocks.length }}</el-button>
          <el-button icon="el-icon-arrow-down" @click="changeConflictBlockBy(1)"
            >下一个冲突块</el-button
          >
        </el-button-group>
        <div class="progress-bar">
          <el-progress
            :text-inside="true"
            :stroke-width="40"
            :percentage="
              conflictBlocks.length == 0
                ? 0
                : (100 * reviewedBlockCount) / conflictBlocks.length
            "
            :format="progressBarFormat"
          ></el-progress>
        </div>
      </div>

      <div>
        <span style="margin-right: 12px">{{
          this.$route.params.filePath
        }}</span>
        <el-button
          class="btn-change-view"
          size="medium"
          title="点击切换新版视图"
          @click="handleSwitchViewClicked"
        >
          <svg
            xmlns="http://www.w3.org/2000/svg"
            width="12"
            height="12"
            viewBox="0 0 2048 2048"
          >
            <path
              fill="#606266"
              d="M1274 1018q-11 57-39 105t-71 83t-94 54t-110 20H576q-32 0-61 10t-53 28t-42 43t-27 56q54 13 99 42t78 71t51 92t19 106q0 66-25 124t-69 102t-102 69t-124 25q-66 0-124-25t-101-68t-69-102t-25-125q0-57 19-108t52-94t81-71t103-40V633q-56-11-103-40t-80-71t-53-93T1 320q0-66 25-124T94 95t102-69T320 0q66 0 124 25t101 69t69 102t26 124q0 57-19 109t-53 93t-81 71t-103 40v585q42-32 91-49t101-17h384q32 0 61-10t53-28t42-43t27-56q-54-13-99-42t-78-70t-51-92t-19-107q0-66 25-124t68-101t102-69t125-26q66 0 124 25t101 69t69 102t26 124q0 58-19 110t-55 94t-83 71t-105 39zm-250-314q0 40 15 75t41 61t61 41t75 15q40 0 75-15t61-41t41-61t15-75q0-40-15-75t-41-61t-61-41t-75-15q-40 0-75 15t-61 41t-41 61t-15 75zM128 320q0 40 15 75t41 61t61 41t75 15q40 0 75-15t61-41t41-61t15-75q0-40-15-75t-41-61t-61-41t-75-15q-40 0-75 15t-61 41t-41 61t-15 75zm384 1408q0-40-15-75t-41-61t-61-41t-75-15q-40 0-75 15t-61 41t-41 61t-15 75q0 40 15 75t41 61t61 41t75 15q40 0 75-15t61-41t41-61t15-75zm1216-704q66 0 124 25t101 69t69 102t26 124q0 66-25 124t-69 102t-102 69t-124 25q-47 0-92-13t-84-40l-419 418q-19 19-45 19t-45-19t-19-45q0-26 19-45l418-419q-26-39-39-84t-14-92q0-66 25-124t68-101t102-69t125-26zm0 512q40 0 75-15t61-41t41-61t15-75q0-40-15-75t-41-61t-61-41t-75-15q-40 0-75 15t-61 41t-41 61t-15 75q0 40 15 75t41 61t61 41t75 15z"
            />
          </svg>
          新版视图
        </el-button>
        <el-button icon="el-icon-circle-check" @click="submitClicked"
          >提交</el-button
        >
        <el-button icon="el-icon-refresh" @click="resetClicked">重置</el-button>
      </div>
    </el-header>

    <el-main>
      <el-row class="threeEditorRow">
        <el-col :span="23">
          <el-col :span="8">
            <div>
              <el-button
                size="mini"
                icon="el-icon-caret-bottom"
                @click="flipLeft"
              ></el-button>
              <span>目标分支</span>
            </div>
            <div v-show="leftShow">
              <MonacoEditor
                :language="language"
                :options="readOnlyEditorOption"
                :theme="editorTheme"
                v-model="left"
                class="editor"
                ref="leftEditor"
              ></MonacoEditor>
            </div>
          </el-col>

          <el-col :span="8">
            <div>
              <el-button
                size="mini"
                icon="el-icon-caret-bottom"
                @click="flipBase"
              ></el-button>
              <span>基础分支</span>
            </div>
            <div v-show="baseShow">
              <MonacoEditor
                :language="language"
                :options="readOnlyEditorOption"
                :theme="editorTheme"
                v-model="base"
                class="editor"
                ref="baseEditor"
              ></MonacoEditor>
            </div>
          </el-col>

          <el-col :span="8">
            <div>
              <el-button
                size="mini"
                icon="el-icon-caret-bottom"
                @click="flipRight"
              ></el-button>
              <span>源分支</span>
            </div>
            <div v-show="rightShow">
              <MonacoEditor
                :language="language"
                :options="readOnlyEditorOption"
                :theme="editorTheme"
                v-model="right"
                class="editor"
                ref="rightEditor"
              ></MonacoEditor>
            </div>
          </el-col>
        </el-col>

        <el-col :span="1">
          <div class="myBar">
            <el-slider
              v-model="blockValue"
              vertical
              height="40vh"
              :show-tooltip="false"
              :max="tempMax"
            >
            </el-slider>
          </div>
        </el-col>
      </el-row>

      <el-row>
        <label for="selector" class="radio-label">应用方案：</label>
        <el-radio-group
          id="applied_solution"
          v-model="radio"
          :disabled="this.ifModifyDisabled"
          @change="radioChanged"
        >
          <el-radio-button label="target">目标分支</el-radio-button>
          <el-radio-button label="base">基础分支</el-radio-button>
          <el-radio-button label="source">源分支</el-radio-button>
          <el-radio-button label="history1">历史记录1</el-radio-button>
          <el-radio-button label="history2">历史记录2</el-radio-button>
          <el-radio-button label="history3">历史记录3</el-radio-button>
          <el-radio-button label="recommend_ml">
            推荐方案-ml:
            <span v-if="predictLabel == 'V1'"> 目标分支 </span>
            <span v-if="predictLabel == 'V2'"> 源分支 </span>
            <span v-if="predictLabel == 'CC'"> 拼接 </span>
            <span v-if="predictLabel == 'CB'"> 行重排 </span>
            <span v-if="predictLabel == 'NC'"> 新代码 </span>
            (可信度:{{ Number(predictConfidence).toFixed(2) }})
          </el-radio-button>
          <el-radio-button label="recommend_sa">
            推荐方案-sa
            <span v-if="mergeAlgorithmHasResolution">
              <span :title="mergeAlgorithmRes.get(index - 1).desc">
                (可信度:{{
                  mergeAlgorithmRes.get(index - 1).confidence.toFixed(2)
                }})
              </span>
            </span>
          </el-radio-button>
        </el-radio-group>
        <el-button
          @click="viewContentDialogClicked"
          style="margin-left: 40px"
          type="primary"
          plain
          >{{
            this.ifAllReviewed
              ? "当前文件全文（可编辑）"
              : "当前文件全文（只读）"
          }}</el-button
        >
        <el-dialog
          :visible.sync="viewContentDialogVisible"
          width="70%"
          :title="
            this.ifAllReviewed
              ? '当前文件全文（可编辑）'
              : '当前文件全文（只读）'
          "
        >
          {{
            this.ifAllReviewed
              ? "当前文件全文（可编辑）"
              : "只读，所有冲突块解决完毕后可编辑"
          }}
          <span>
            <MonacoEditor
              :language="language"
              :theme="editorTheme"
              v-model="viewContentDialogAnswer"
              :options="fileContentEditorOptions"
              class="editor"
            ></MonacoEditor>
          </span>
          <span slot="footer" v-show="ifAllReviewed" class="dialog-footer">
            <el-button @click="dialogEditCancel()">放弃编辑</el-button>
            <el-button
              type="primary"
              @click="dialogEditSave()"
              :disabled="
                this.dialogContentBeforeEdit === this.viewContentDialogAnswer
              "
              >保存编辑（保存后不能修改冲突块）</el-button
            >
          </span>
        </el-dialog>
      </el-row>

      <el-row>
        <el-col :span="23">
          <el-col :span="12">
            合并结果
            <MonacoEditor
              :language="language"
              :theme="editorTheme"
              v-model="merged"
              :options="readOnlyEditorOption"
              class="editor"
              ref="mergeEditor"
            ></MonacoEditor>
          </el-col>
          <el-col :span="12">
            应用内容
            <MonacoEditor
              :language="language"
              :theme="editorTheme"
              v-model="applied"
              :options="appliedEditorOptions"
              class="editor"
              ref="appliedEditor"
            ></MonacoEditor>
          </el-col>
        </el-col>
      </el-row>
    </el-main>
  </el-container>
</template>

<script>
import MonacoEditor from "../components/vue-monaco";
import { set } from "vue";
import qs from "qs";
import { getSourceLanguage } from "@/utils/index";
import { FileTreeView } from "@/router";
import { DiffResolutionView } from "@/router";
import { SA_BASE_URL } from "@/api/config";

const monaco = require("monaco-editor/esm/vs/editor/editor.api");
const MergeAlgorithmStatusType = {
  norun: "norun",
  pending: "pending",
  success: "success",
  fail: "fail",
};

export default {
  name: "BlockResolution",
  components: { MonacoEditor },
  data() {
    return {
      viewContentDialogVisible: false, // “当前全文” 是否可见
      predictData: [], // 分类器对当前文件所有冲突块预测结果
      predictConfidence: 0, // 分类器对当前冲突块预测结果的可信度
      predictLabel: "", // 分类器对当前冲突块预测结果的标签
      radio: "", // 选中的推荐方案索引
      allSelectedRadios: [], // 所有冲突块选中的推荐方案索引
      // monaco options
      language: "", // 文件语言
      readOnlyEditorOption: {
        readOnly: true,
      },

      // theme
      overallTheme: "white",
      sidebarTheme: "white-theme", // black-theme
      editorTheme: "vs-light", // vs-dark

      // monaco instance
      mergeEditor: null,
      decorations: null,

      // data
      index: 1, // 冲突索引，start from 1
      conflictBlocks: [], // start from 0
      filePath: "",
      left: "", // 目标分支内容
      base: "", // 基础分支内容
      right: "", // 源分支内容
      merged: "", // 合并结果
      applied: "", // 当前冲突块的应用内容
      fileName: "", // 文件名
      repo: "", // 仓库名

      leftShow: true, // editor显示控制
      baseShow: true,
      rightShow: true,
      blockValue: 100, // 控制editor滑动高度
      tempMax: 100, // 控制editor滑动高度

      appliedContents: [],

      tempPath: "", // 临时文件路径
      targetPath: "", // 目标分支文件路径
      sourcePath: "", // 源分支文件路径
      basePath: "", // 基础分支文件路径
      conflictPath: "", // 冲突文件路径

      targetBranch: "", // 目标分支名称
      sourceBranch: "", // 源分支名称
      block_info: [], // 冲突块相关内容，包括起止位置、历史结果、各部分内容等
      viewContentDialogAnswer: "", // 当前文件全文预览

      // WHU merge algorithm result
      mergeAlgorithmRes: new Map(),
      mergeAlgorithmResChangeTracker: 1,
      mergeAlgorithmStatus: MergeAlgorithmStatusType.norun,
      // mergeAlgorithmHasResolution: mergeAlgorithmRes.has(index - 1),

      ifModifyDisabled: false, // 是否禁止修改
      dialogContentBeforeEdit: "", // 每次对话框编辑前的文件内容
    };
  },
  //production version created
  async created() {
    this.fileName = this.$route.params.fileName;
    this.language = getSourceLanguage(this.fileName.split(".").pop());
    this.filePath = this.$route.params.filePath;
    this.repo = this.$route.params.repo;
    this.targetBranch = this.$route.params.target;
    this.sourceBranch = this.$route.params.source;
    await this.getSpecifiedConflict();
    await this.getClassifier();
    await this.getMergeAlgorithmResult();
    this.scanConflictBlocks();
    this.refreshData();
  },
  mounted() {
    this.mergeEditor = this.$refs.mergeEditor.getEditor();
    this.leftEditor = this.$refs.leftEditor.getEditor();
    this.baseEditor = this.$refs.baseEditor.getEditor();
    this.rightEditor = this.$refs.rightEditor.getEditor();
  },
  watch: {
    blockValue() {
      let lh = this.leftEditor.getScrollHeight() - 301;
      let mh = this.baseEditor.getScrollHeight() - 301;
      let rh = this.rightEditor.getScrollHeight() - 301;
      this.tempMax = Math.max(lh, mh, rh, 0);
      this.leftEditor.setScrollTop(this.tempMax - this.blockValue);
      this.baseEditor.setScrollTop(this.tempMax - this.blockValue);
      this.rightEditor.setScrollTop(this.tempMax - this.blockValue);
    },
    index() {
      this.applied = this.appliedContents[this.index - 1]; // 第 index 个冲突的应用内容
      if (this.predictData.length > 0) {
        this.predictLabel = this.predictData[this.index - 1].label;
        this.predictConfidence = this.predictData[this.index - 1].confidence;
      }
    },
    applied() {
      this.appliedContents[this.index - 1] = this.applied;
    },
  },
  computed: {
    mergeAlgorithmHasResolution() {
      // const x = this.mergeAlgorithmResChangeTracker;
      return this.mergeAlgorithmRes.has(this.index - 1);
    },
    fileContentEditorOptions() {
      return {
        readOnly: !this.ifAllReviewed,
      };
    },
    appliedEditorOptions() {
      return {
        readOnly: this.ifModifyDisabled,
      };
    },
    reviewedBlockCount() {
      return this.allSelectedRadios.filter(
        radio => radio !== "conflict_content",
      ).length;
    },
    ifAllReviewed() {
      // 如果所有应用方案都已选择，可编辑提交内容，但是还没有禁止修改，一旦内容也被修改（ifModifyDisabled），禁用修改
      return this.reviewedBlockCount === this.conflictBlocks.length;
    },
  },
  methods: {
    handleSwitchViewClicked() {
      this.$router.push({
        name: DiffResolutionView,
        query: this.$route.query,
        params: {
          fileName: this.fileName,
          filePath: this.filePath,
          repo: this.repo,
          target: this.targetBranch,
          source: this.sourceBranch,
        },
      });
    },
    progressBarFormat() {
      return `已解决 ${this.reviewedBlockCount} / ${this.conflictBlocks.length}`;
    },
    viewContentDialogClicked() {
      this.viewContentDialogVisible = true;
      if (!this.ifModifyDisabled) {
        // 还没有修改过，使用生成的文件内容
        this.viewContentDialogAnswer = this.generateFileContent();
      }
      this.dialogContentBeforeEdit = this.viewContentDialogAnswer; // 更新编辑前的文件内容
    },

    dialogEditSave() {
      this.viewContentDialogVisible = false;
      this.ifModifyDisabled = true; // 禁止修改
    },

    dialogEditCancel() {
      this.viewContentDialogVisible = false;
      this.viewContentDialogAnswer = this.dialogContentBeforeEdit; // 恢复编辑前的文件内容
    },

    async resetClicked() {
      await this.refreshData();
      this.viewContentDialogAnswer = this.generateFileContent();
      this.ifModifyDisabled = false; // 允许修改
      // 滚动到冲突块起始行
      this.mergeEditor.revealLineInCenter(
        this.conflictBlocks[this.index - 1].right_start,
      );
    },

    async getClassifier() {
      const fileExt = this.filePath.split(".").pop();
      if (this.language !== "unknown") {
        try {
          const resp = await this.$axios({
            method: "post",
            headers: {
              "Content-Type": "application/json",
            },
            data: {
              path: this.repo,
              target: this.targetBranch,
              source: this.sourceBranch,
              version1: this.targetPath.replace(/\\/g, "/"),
              version2: this.sourcePath.replace(/\\/g, "/"),
              conflict: this.conflictPath.replace(/\\/g, "/"),
              base: this.basePath.replace(/\\/g, "/"),
              filetype: this.language,
            },
            url: "http://127.0.0.1:5001/predict",
          });

          if (resp && resp.status === 200) {
            this.predictData = resp.data.data;
            this.predictLabel = this.predictData[this.index - 1].label;
            this.predictConfidence =
              this.predictData[this.index - 1].confidence;
          }
        } catch (e) {
          this.$message({
            message: "classifier调用失败：" + e,
            type: "warning",
            showClose: true,
          });
        }
      } else {
        this.$message({
          message: `${fileExt} 文件类型，分类预测结果未获取`,
          type: "warning",
          showClose: true,
        });
      }
    },
    async callMergeAlgorithmApi() {
      if (this.language !== "cpp") {
        this.mergeAlgorithmStatus = MergeAlgorithmStatusType.norun;
        return;
      }
      this.$axios
        .post(`${SA_BASE_URL}/resolve`, {
          path: this.repo,
          ms: {
            ours: this.targetBranch,
            theirs: this.sourceBranch,
          },
          file: this.filePath,
        })
        .then(res => {
          // 接口请求成功
          if (res.data.code === "00000") {
            // 算法执行成功

            const resolutions = res.data.data.resolutions;

            resolutions.forEach(resolution => {
              this.mergeAlgorithmRes.set(resolution.index, resolution);
              this.mergeAlgorithmResChangeTracker += 1;
            });

            if (res.data.data.pending) {
              // 算法执行未完成
              this.mergeAlgorithmStatus = MergeAlgorithmStatusType.pending;
            } else {
              // 算法执行完成
              this.mergeAlgorithmStatus = MergeAlgorithmStatusType.success;
            }
          } else {
            // 算法执行失败
            this.mergeAlgorithmStatus = MergeAlgorithmStatusType.fail;

            this.$message({
              showClose: true,
              message: `算法执行失败：${res.data.msg}`,
              type: "warning",
            });
          }
        })
        .catch(err => {
          this.mergeAlgorithmStatus = MergeAlgorithmStatusType.fail;

          // 接口请求失败
          let errorMsg = "未知原因";
          if (err.response === null || err.response === undefined) {
            errorMsg = err.message;
          } else {
            errorMsg = err.response.data.msg;
          }

          this.$message({
            showClose: true,
            message: `接口（/api/sa/resolve）请求失败：${errorMsg}`,
            type: "warning",
          });
        });
    },
    async getMergeAlgorithmResult() {
      await this.callMergeAlgorithmApi();

      // not c/cpp conflict files, skip polling
      if (this.mergeAlgorithmStatus == MergeAlgorithmStatusType.norun) {
        return;
      }

      // todo: clear up
      const apiCallTimer = setInterval(() => {
        if (
          this.mergeAlgorithmStatus === MergeAlgorithmStatusType.success ||
          this.mergeAlgorithmStatus === MergeAlgorithmStatusType.fail
        ) {
          clearInterval(apiCallTimer);
        } else {
          this.callMergeAlgorithmApi();
        }
      }, 5000);
    },
    async radioChanged() {
      const i = this.index - 1;
      if (this.radio == "target") {
        this.appliedContents[i] = this.conflictBlocks[i].left_code.join("\n");
      } else if (this.radio == "base") {
        this.appliedContents[i] = this.conflictBlocks[i].base_code.join("\n");
      } else if (this.radio == "source") {
        this.appliedContents[i] = this.conflictBlocks[i].right_code.join("\n");
      } else if (this.radio == "recommend_ml") {
        if (this.predictData[i].label == "V1") {
          this.appliedContents[i] = this.conflictBlocks[i].left_code.join("\n");
        } else if (this.predictData[i].label == "V2") {
          this.appliedContents[i] =
            this.conflictBlocks[i].right_code.join("\n");
        } else if (this.predictData[i].label == "CC") {
          this.appliedContents[i] =
            this.conflictBlocks[i].left_code.join("\n") +
            "\n" +
            this.conflictBlocks[i].right_code.join("\n");
        } else if (this.predictData[i].label == "CB") {
          try {
            const { data: resp } = await this.$axios({
              method: "post",
              headers: { "Content-Type": "application/json" },
              data: {
                version1: this.conflictBlocks[i].left_code.join("\n"),
                version2: this.conflictBlocks[i].right_code.join("\n"),
                base: this.conflictBlocks[i].base_code.join("\n"),
              },
              url: "http://127.0.0.1:3001/combine",
            });
            if (resp && resp.isSuccessful)
              this.appliedContents[i] = resp.data.data;
            else throw resp.msg;
          } catch (e) {
            this.$message({
              showClose: true,
              message: "行重排失败：" + e,
              type: "warning",
            });
          }
        } else if (this.predictData[i].label == "NC") {
          this.appliedContents[i] =
            ["<<<<<<<"]
              .concat(this.conflictBlocks[i].left_code)
              .concat("|||||||")
              .concat(this.conflictBlocks[i].base_code)
              .concat("=======")
              .concat(this.conflictBlocks[i].right_code)
              .concat(">>>>>>>")
              .join("\n") + "\n";
        }
      } else if (this.radio == "recommend_sa") {
        if (this.mergeAlgorithmRes.has(i)) {
          this.appliedContents[i] = this.mergeAlgorithmRes
            .get(i)
            .code.join("\n");
        } else {
          this.appliedContents[i] = "";
        }
      }

      this.applied = this.appliedContents[i];
      set(this.allSelectedRadios, i, this.radio); // 使 computed 属性可以更新
    },
    flipLeft() {
      this.leftShow = !this.leftShow;
    },
    flipBase() {
      this.baseShow = !this.baseShow;
    },
    flipRight() {
      this.rightShow = !this.rightShow;
    },
    generateFileContent() {
      let lines = this.merged.split(/\r?\n/);
      let resolution = "";
      let collected_lines = [];
      let resolution_idx = 0;
      let i = 0;
      while (i < lines.length) {
        if (lines[i].startsWith("<<<<<<<")) {
          // 写入之前的内容
          resolution += collected_lines.join("\n") + "\n";
          collected_lines = []; // 清空
          // 写入当前冲突块的解决方案
          resolution += this.appliedContents[resolution_idx++] + "\n";
          for (; i < lines.length; i++) {
            if (lines[i].startsWith(">>>>>>>")) break;
          }
        } else {
          collected_lines.push(lines[i]); // 普通行
        }
        i += 1;
      }
      if (collected_lines.length != 0) {
        resolution += collected_lines.join("\n") + "\n";
      }
      // 去掉最后一个换行符
      resolution = resolution.substring(0, resolution.length - 1);
      return resolution;
    },
    submitClicked() {
      if (this.reviewedBlockCount < this.conflictBlocks.length) {
        this.$message({
          showClose: true,
          message: "还有未解决的冲突块",
          type: "warning",
        });
        return;
      }

      const content = this.ifModifyDisabled
        ? this.viewContentDialogAnswer
        : this.generateFileContent();
      this.$axios.put(
        "/write2file",
        qs.stringify({
          // 后端没有使用 @RequestBody，所以这里需要 qs.stringify，符合 application/x-www-form-urlencoded 格式
          path: this.filePath,
          fileName: this.fileName,
          content: content,
          repo: this.repo,
          tempPath: this.tempPath,
        }),
      );

      this.$message({
        showClose: true,
        message: "已提交",
        type: "success",
      });
    },

    previousPage() {
      this.$router.back();
    },
    async getSpecifiedConflict() {
      await this.$axios
        .get(
          "/conflict/specified?filePath=" +
            this.filePath +
            "&repo=" +
            this.repo +
            "&fileType=0",
        )
        .then(resp => {
          if (resp && resp.status === 200) {
            // this.data = resp.data.data;
            const {
              data: {
                ours,
                base,
                theirs,
                conflict,
                tempPath,
                targetPath,
                sourcePath,
                basePath,
                conflictPath,
              },
              info: block_info,
            } = resp.data;

            this.left = ours || "";
            this.base = base || "";
            this.right = theirs || "";
            this.merged = conflict.join("\n") || "";
            this.tempPath = tempPath;
            this.targetPath = targetPath;
            this.sourcePath = sourcePath;
            this.basePath = basePath;
            this.conflictPath = conflictPath;
            this.block_info = block_info;
          }
        });
    },

    /*
     * scan the merged code to get the map between index and conflict blocks start line number
     * and highlight the conflicting lines
     * only call when initializing for it refresh the conflictBlocks
     */
    scanConflictBlocks() {
      this.conflictBlocks = [];
      for (const i in this.block_info) {
        const block = {
          radio: "conflict_content", // 默认不选中应用内容
        };
        block.left_start = parseInt(this.block_info[i].start);
        block.base_start = parseInt(this.block_info[i].middle);
        block.right_start = parseInt(this.block_info[i].middle2);
        block.right_end = parseInt(this.block_info[i].end);
        block.left_code = this.block_info[i].ours;
        block.base_code = this.block_info[i].base;
        block.right_code = this.block_info[i].theirs;
        this.conflictBlocks.push(block);
      }
      this.mergeEditor.revealLineInCenter(this.conflictBlocks[0].right_start);

      // highlight conflicting lines
      let decorations = [];
      function getDecoration(startLine, endLine, className) {
        return {
          range: new monaco.Range(startLine, 0, endLine, 0),
          options: {
            isWholeLine: true,
            className: className,
            marginClassName: className,
          },
        };
      }
      this.conflictBlocks.forEach(conflictBlock => {
        if (conflictBlock.base_start > 0) {
          // diff3
          decorations.push(
            getDecoration(
              conflictBlock.left_start,
              conflictBlock.base_start - 1,
              "leftLineDecoration",
            ),
          );
          decorations.push(
            getDecoration(
              conflictBlock.base_start,
              conflictBlock.right_start - 1,
              "baseLineDecoration",
            ),
          );
        } else {
          // diff2
          decorations.push(
            getDecoration(
              conflictBlock.left_start,
              conflictBlock.right_start - 1,
              "leftLineDecoration",
            ),
          );
        }
        decorations.push(
          getDecoration(
            conflictBlock.right_start,
            conflictBlock.right_end,
            "rightLineDecoration",
          ),
        );
      });

      this.mergeEditor.deltaDecorations(
        this.mergeEditor.getModel().getAllDecorations(),
        decorations,
      );
    },

    /*
     * refresh the conflict blocks and show the first one
     */
    refreshData() {
      this.appliedContents = [];
      const block2string = block => {
        return (
          "<<<<<<<\n" +
          block.left_code.join("\n") +
          "\n|||||||\n" +
          block.base_code.join("\n") +
          "\n=======\n" +
          block.right_code.join("\n") +
          "\n>>>>>>>"
        );
      };
      for (const i in this.conflictBlocks) {
        this.appliedContents.push(block2string(this.conflictBlocks[i]));
      }
      this.allSelectedRadios = new Array(this.conflictBlocks.length).fill(
        "conflict_content",
      );
      this.index = 1;
      this.applied = this.appliedContents[this.index - 1];
      this.radio = this.allSelectedRadios[this.index - 1];
    },

    /*
     * change the conflict block by offset
     * @param offset: the offset of the conflict block to be changed
     */
    changeConflictBlockBy(offset) {
      this.index =
        Math.abs((this.index - 1 + offset) % this.conflictBlocks.length) + 1;
      // 滚动到冲突块起始行
      this.mergeEditor.revealLineInCenter(
        this.conflictBlocks[this.index - 1].right_start,
      );
      this.radio = this.allSelectedRadios[this.index - 1];
    },

    handleBackClicked() {
      this.$router.push({ name: FileTreeView, query: this.$route.query });
    },
  },
};
</script>

<style scoped>
.el-container {
  height: 100vh;
  width: 100vw;
  overflow: hidden;
}

.el-header {
  background-color: #b3c0d1;
  display: flex;
  align-items: center;
  justify-content: space-between;
  line-height: 60px;
}

.left-area {
  display: flex;
  gap: 20px;
  align-items: center;
}

.progress-bar {
  width: 200px;
  height: 40px;
}

.el-main {
  background-color: #e9eef3;
  color: #333;
  overflow: auto;
  padding: 20px 0 20px 50px;
}

.el-row {
  margin: 6px 0;
}

.threeEditorRow {
  display: flex;
  align-items: flex-end;
}

.editor {
  height: 40vh;
  overflow: hidden;
  margin: 0px;
  padding: 0px;
  border: 1px solid grey;
}
</style>

<style>
.leftLineDecoration {
  background: lightblue;
}

.baseLineDecoration {
  background: lightgrey;
}

.rightLineDecoration {
  background: pink;
}
</style>
