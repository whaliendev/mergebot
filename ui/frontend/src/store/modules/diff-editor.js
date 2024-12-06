import classifier from "@/api/classifier";
import { getEditScriptResolution } from "@/api/edit_script_resolver";
import SA from "@/api/sa";
import { findConflictBlocks } from "@/utils/merge";
import Vue from "vue";
import {
  getBlockHash,
  findSpecificFingerprint,
  getArraySlice,
} from "@/utils/merge";
import {
  constructSemanticPatch,
  customGitDiff2 as customGitDiff,
} from "@/utils/git";
import { saveBlockResolutionChoice } from "@/api/audit";

const StartMark = "<<<<<<<";
const BasesMark = "|||||||";
const TheirMark = "=======";
const EndMark = ">>>>>>>";

/**
 * Converts a block object into a conflict chunk string.
 * @param block - The block object containing 'ours', 'base', and 'theirs' properties.
 * @returns {string} The conflict chunk string.
 */
const block2ConflictChunk = block =>
  // todo: Add support for the properties after conflict markers.
  [StartMark]
    .concat(block.ours)
    .concat(BasesMark)
    .concat(block.base)
    .concat(TheirMark)
    .concat(block.theirs)
    .concat(EndMark)
    .join("\n");

const state = () => ({
  editor: {
    // reset when enter any diff editor
    original: [""], // readonly, lines
    modified: [""],
    rtModified: [""],

    undoStack: [],
    redoStack: [],

    originalFingerprints: [""], // readonly, SHA256s,
    conflictBlocks: [],
    locationMapping: new Map(), // key: applyPayload, value: block index
  },

  file: {
    projectPath: "",
    targetBranch: "",
    sourceBranch: "",
    fileName: "",
    filePath: "",
  },

  git: {
    resolutions: [
      {
        index: 1,
        histories: [{ res: "", confidence: 0 }], // always 3 histories
      }, // resolution count equals to conflict block count
    ],
  },

  ml: {
    resolutions: [
      {
        index: 1, // 1-based
        resolution: "",
        confidence: 0,
      },
    ],
  },

  dl: {
    isPending: true,
    resolutions: [
      {
        index: 1, // 1-based
        resolvable: false,
        resolution: "",
      },
    ],
  },

  sa: {
    pending: false,
    nopatches: true,
    timeout: false,
    resolutions: [
      {
        desc: "",
        confidence: 0,
        index: 1, // 1-based
        label: "",
        resolution: "",
      },
    ],
    merged: [], // readonly lines
    patches: [],
    constPatches: [],
    // patchStartLine: -1,
  },

  patchStartLine: -1,
  remainingConflictBlocks: [],
});

const getters = {
  hoverContentForBlock(state) {
    return (index, label) => {
      if (index < 0 || index >= state.editor.conflictBlocks.length) return "";
      const block = state.editor.conflictBlocks[index];
      // console.log(index);
      // console.log(block);
      if (block.baseMarkerLineNo !== -1) {
        // three way
        if (label === "our") {
          return state.editor.original
            .slice(block.ourMarkerLineNo, block.baseMarkerLineNo - 1)
            .join("\n");
        } else if (label === "base") {
          return state.editor.original
            .slice(block.baseMarkerLineNo, block.theirMarkerLineNo - 1)
            .join("\n");
        } else if (label === "their") {
          return state.editor.original
            .slice(block.theirMarkerLineNo, block.endMarkerLineNo - 1)
            .join("\n");
        }
      } else {
        if (label === "our") {
          return state.editor.original
            .slice(block.ourMarkerLineNo, block.theirMarkerLineNo - 1)
            .join("\n");
        } else if (label === "their") {
          return state.editor.original
            .slice(block.theirMarkerLineNo, block.endMarkerLineNo - 1)
            .join("\n");
        }
      }
      return "";
    };
  },
  blockResolutions: state => {
    return [
      state.git.resolutions,
      state.ml.resolutions,
      state.sa.resolutions,
      state.dl.resolutions,
    ];
  },
  originalContent: state => {
    return state.editor.original.join("\n");
  },
  modifiedContent: state => {
    // debugger;
    return state.editor.modified.join("\n");
  },
  canUndo: state => {
    return state.editor.undoStack.length > 0;
  },
  canRedo: state => {
    return state.editor.redoStack.length > 0;
  },
  patches: state => {
    return state.sa.patches;
  },
  rtResolvedFileContent: state => {
    return state.editor.rtModified;
  },
  remainingConflictBlocksCnt: state => {
    return state.remainingConflictBlocks.length;
  },

  getBlockResolutionChoice: state => payload => {
    const blockCode = {
      choiceCode: "",
      saCode: "",
      mlCode: "",
      dlCode: "",
    };

    const { index: resoIndex, type, fingerIndex: blockIdx } = payload;
    // if type is ours, theirs, base, we need to fill saCode, mlCode and dlCode.
    // otherwise, we only need to fill choiceCode.
    // type domain: ['ours', 'base', 'theirs', 'sa', 'patch', 'ml', 'dl']
    if (["ours", "base", "theirs"].includes(payload.type)) {
      if (type === "ours") {
        const block = state.editor.conflictBlocks[resoIndex];

        const ourStart = block.ourMarkerLineNo;
        const ourEnd =
          block.baseMarkerLineNo === -1
            ? block.theirMarkerLineNo
            : block.baseMarkerLineNo;
        const ourSlice = getArraySlice(
          state.editor.original,
          ourStart - 1 + 1,
          ourEnd - 1 - 1,
        );
        blockCode.choiceCode = ourSlice.join("\n");
      } else if (type === "base") {
        const block = state.editor.conflictBlocks[resoIndex];

        const baseStart = block.baseMarkerLineNo;
        const baseEnd = block.theirMarkerLineNo;
        if (baseStart === -1 || baseEnd === -1)
          throw new Error("当前冲突区块无base版本解决方案");
        const baseSlice = getArraySlice(
          state.editor.original,
          baseStart + 1 - 1,
          baseEnd - 1 - 1,
        );
        blockCode.choiceCode = baseSlice.join("\n");
      } else if (type === "theirs") {
        const block = state.editor.conflictBlocks[resoIndex];

        const theirStart = block.theirMarkerLineNo;
        const theirEnd = block.endMarkerLineNo;
        const theirSlice = getArraySlice(
          state.editor.original,
          theirStart + 1 - 1,
          theirEnd - 1 - 1,
        );
        blockCode.choiceCode = theirSlice.join("\n");
      }

      // fill saCode, mlCode and dlCode
      // saCode
      const saResoIdx = state.sa.resolutions.findIndex(reso => {
        return reso.index === blockIdx + 1;
      });
      if (saResoIdx === -1) {
        blockCode.saCode = "";
      } else {
        blockCode.saCode = state.sa.resolutions[saResoIdx].resolution;
      }

      const conflictBlock = state.editor.conflictBlocks[blockIdx];
      // const patchResoIdx = state.sa.constPatches.findIndex(patch => {
      //   const patchStart = patch.start;
      //   const patchEnd = patch.start + patch.offset - 1;
      //   const blockStart = conflictBlock.ourMarkerLineNo;
      //   const blockEnd = conflictBlock.endMarkerLineNo;
      //   return (
      //     (patchStart <= blockStart && patchEnd >= blockStart) ||
      //     (patchStart > blockStart && patchStart <= blockEnd)
      //   );
      // });
      let patchResoIdx = -1;
      let maxIntersect = 0;
      state.sa.constPatches.forEach((patch, idx) => {
        const patchStart = patch.start;
        const patchEnd = patch.start + patch.offset - 1;
        const blockStart = conflictBlock.ourMarkerLineNo;
        const blockEnd = conflictBlock.endMarkerLineNo;
        let intersect = 0;
        if (patchStart <= blockStart && patchEnd >= blockStart) {
          intersect = Math.min(patchEnd, blockEnd) - blockStart + 1;
        } else if (patchStart > blockStart && patchStart <= blockEnd) {
          intersect = Math.min(patchEnd, blockEnd) - patchStart + 1;
        }
        if (intersect > maxIntersect) {
          maxIntersect = intersect;
          patchResoIdx = idx;
        }
      });
      if (patchResoIdx !== -1) {
        blockCode.saCode = state.sa.constPatches[patchResoIdx].newContent;
      }

      // mlCode
      const mlResoIdx = state.ml.resolutions.findIndex(reso => {
        return reso.index === blockIdx + 1;
      });
      if (mlResoIdx === -1) {
        blockCode.mlCode = "";
      } else {
        blockCode.mlCode = state.ml.resolutions[mlResoIdx].resolution;
      }

      // dlCode
      if (
        !state.dl.isPending &&
        state.dl.resolutions.length > 0 &&
        blockIdx < state.dl.resolutions.length &&
        state.dl.resolutions[blockIdx].resolvable
      ) {
        let res = state.dl.resolutions[blockIdx];
        if (res.resolution[res.resolution.length - 1] === "\n") {
          res.resolution = res.resolution.slice(0, -1);
        }
        blockCode.dlCode = res.resolution;
      }
    } else {
      if (["sa", "patch"].includes(type)) {
        if (type === "sa") {
          if (state.sa.resolutions.length <= resoIndex)
            throw new Error("当前冲突区块无SA解决方案");

          const res = state.sa.resolutions[resoIndex];
          blockCode.saCode = res.resolution;
        } else if (type === "patch") {
          const patch = state.sa.patches[resoIndex];
          blockCode.saCode = patch.newContent;
        }
        blockCode.choiceCode = blockCode.saCode;
      } else if (type === "ml") {
        if (state.ml.resolutions.length <= resoIndex)
          throw new Error("当前冲突区块无ML解决方案");

        const res = state.ml.resolutions[resoIndex];
        blockCode.mlCode = res.resolution;
        blockCode.choiceCode = blockCode.mlCode;
      } else if (type === "dl") {
        if (
          !state.dl.isPending &&
          state.dl.resolutions.length > 0 &&
          resoIndex < state.dl.resolutions.length &&
          state.dl.resolutions[resoIndex].resolvable
        ) {
          let res = state.dl.resolutions[resoIndex];
          if (res.resolution[res.resolution.length - 1] === "\n") {
            res.resolution = res.resolution.slice(0, -1);
          }
          blockCode.dlCode = res.resolution;
        }
        blockCode.choiceCode = blockCode.dlCode;
      }
    }

    return blockCode;
  },
};

const actions = {
  async getDLResolutions({ commit }, payload) {
    commit("setDLResolutions", { isPending: true, resolutions: [] });
    const { blocks } = payload;
    const arr2str = arr => (arr.length > 0 ? arr.join("\n") + "\n" : "");
    const payloads = blocks.map(block => ({
      base: arr2str(block.base),
      ours: arr2str(block.ours),
      theirs: arr2str(block.theirs),
    }));
    // 网络错误交由上层处理
    const ret = await Promise.all(
      payloads.map(payload => getEditScriptResolution(payload)),
    );
    commit("setDLResolutions", {
      resolutions: ret.map((ret, idx) => {
        if (ret.resolvable === true) {
          return {
            idx: idx + 1, // 1-based
            resolvable: true,
            resolution: arr2str(ret.resolution),
          };
        }
        return {
          idx: idx + 1,
          resolvable: false,
          resolution: "",
        };
      }),
      isPending: false,
    });
  },

  async getBlockResolution(_, { label, block }) {
    if (label === "V1") {
      return block.ours.join("\n");
    } else if (label === "V2") {
      return block.theirs.join("\n");
    } else if (label === "CC") {
      return block.ours.join("\n") + "\n" + block.theirs.join("\n");
    } else if (label === "NC") {
      // 拼接冲突块，存在的问题是不能获取到原来 conflict marker 后的标识符
      return block2ConflictChunk(block);
    } else if (label === "CB") {
      return await classifier.getCombineResult({
        version1: block.ours.join("\n"),
        version2: block.theirs.join("\n"),
        base: block.base.join("\n"),
      });
    } else {
      throw new Error(`未预料到的Reolusion类型: ${label}`);
    }
  },
  async getMLResolutions({ commit, dispatch }, payload) {
    // console.log(payload);
    const resList = await classifier.getClassifierPredictionResults(
      payload.payload,
    );
    if (resList) {
      commit("clearMLResolutions");
      resList.forEach(async (res, index) => {
        // console.log(index)
        try {
          const resolution = await dispatch("getBlockResolution", {
            label: res.label,
            block: payload.blocks[index],
          });
          commit("pushMLResolution", {
            index: res.index + 1,
            confidence: res.confidence,
            resolution: resolution,
            label: res.label,
          });
        } catch (error) {
          console.error(error);
          Vue.prototype.$message.error(
            `获取冲突块 ${index} 的 ${res.label} 解决方案失败：${error.message}`,
          );
        }
      });
      // console.log(state.ml.resolutions);
    }
  },

  async getSAResolutions({ commit }, payload) {
    commit("resetSAResolutions");
    const maxAttempts = 7;
    const waitInterval = 5000;

    let currentAttempt = 0;
    commit("setSAResolutionPending", true);
    let res = null;
    while (currentAttempt < maxAttempts) {
      try {
        res = await SA.getSAResolutionResult(payload);
        if (res) {
          if (res.pending) {
            commit("setSAResolutions", res);
            ++currentAttempt;
            await new Promise(resolve => setTimeout(resolve, waitInterval));
            continue;
          } else {
            commit("setSAResolutionPending", false);
            if (!res.merged || res.merged.length === 0) {
              // commit('resetSAResolutions');
              commit("setSAResolutionNoPatches", true);
            } else {
              commit("setSAResolutionNoPatches", false);
            }
            commit("setSAResolutions", res);
            break;
          }
        }
      } catch (error) {
        console.error(error);
        Vue.prototype.$message.error(
          `SA: 获取冲突块${payload.ms.ours}和${payload.ms.theirs}的解决方案失败, ${error.message}`,
        );
        commit("resetSAResolutions"); // if an error occurs, reset the resolutions
        break;
      }
    }

    if (res && res.patches && res.patches.length === 0) {
      commit("setSAResolutionNoPatches", true);
    }

    commit("setSAResolutionPending", false);
    if (currentAttempt === maxAttempts) {
      commit("setSAResolutionTimeout", true);
    }
  },

  async testIsApplicable({ commit, state }, payload) {
    // debugger;
    const { type } = payload;
    if (type !== "patch") {
      const { fingerIndex } = payload;
      const finger = state.editor.originalFingerprints[fingerIndex];

      const blockInterval = findSpecificFingerprint(
        state.editor.rtModified,
        finger,
      );
      // console.log(blockInterval);
      if (blockInterval.start === -1 || blockInterval.end === -1) return false;

      commit("storeLocationMapping", { payload, blockInterval });
      return true;
    } else {
      return true; // patch is always applicable
    }
  },

  async saveBlockResolutionChoice({ getters, state }, payload) {
    // here, our fingerIndex is actually the block index, while index is the resolution index
    let { index: resoIndex, type, fingerIndex: blockIdx } = payload;

    const blockCode = getters.getBlockResolutionChoice(payload);

    if (type === "patch") {
      // in audition, patch is also one of sa resolutions
      type = "sa";
    }

    // 构建请求参数
    const request = {
      projectPath: state.file.projectPath,
      targetBranch: state.file.targetBranch,
      sourceBranch: state.file.sourceBranch,
      fileName: state.file.filePath,
      blockIdx,
      blockResolutionChoice: {
        choice: type,
        choiceCode: blockCode.choiceCode,
        saCode: blockCode.saCode,
        mlCode: blockCode.mlCode,
        dlCode: blockCode.dlCode,
      },
    };
    try {
      await saveBlockResolutionChoice(request);
    } catch (error) {
      this.$message.error(error.message);
    }
  },

  async undoChanges({ state, getters }) {
    if (getters.canUndo) {
      const currentState = state.editor.rtModified.join("\n");
      this.commit("diff/doUndoChanges", currentState);
    }
  },

  async redoChanges({ state, getters }) {
    if (getters.canRedo) {
      const currentState = state.editor.rtModified.join("\n");
      this.commit("diff/doRedoChanges", currentState);
    }
  },
};

const mutations = {
  doUndoChanges(state, current) {
    state.editor.redoStack.push(current);
    state.editor.rtModified = state.editor.undoStack.pop().split("\n");
    state.editor.modified = state.editor.rtModified;

    state.sa.patches = constructSemanticPatch(
      state.editor.rtModified,
      customGitDiff(
        state.editor.rtModified.join("\n"),
        state.sa.merged.join("\n"),
      ),
    );
  },

  doRedoChanges(state, current) {
    state.editor.undoStack.push(current);
    state.editor.rtModified = state.editor.redoStack.pop().split("\n");
    state.editor.modified = state.editor.rtModified;

    state.sa.patches = constructSemanticPatch(
      state.editor.rtModified,
      customGitDiff(
        state.editor.rtModified.join("\n"),
        state.sa.merged.join("\n"),
      ),
    );
  },

  saveHistoryResolutions(state, blocks) {
    state.git.resolutions.splice(0, state.git.resolutions.length);
    blocks.forEach((block, index) => {
      let historyResos = [];
      for (let i = 0; i < 2; ++i) {
        if (i < block.historyTruth.length && i < block.truthScore.length) {
          historyResos.push({
            res: block.historyTruth[i],
            confidence: block.truthScore[i],
          });
        } else {
          historyResos.push({
            res: null,
            confidence: -1,
          });
        }
      }

      historyResos = historyResos
        .filter(reso => {
          return reso.res && reso.confidence > 0;
        })
        .sort((a, b) => {
          return b.confidence - a.confidence;
        });

      state.git.resolutions.push({
        index: index + 1,
        histories: historyResos,
      });
    });

    // console.log(state.git.resolutions);
  },

  clearMLResolutions(state) {
    state.ml.resolutions.splice(0, state.ml.resolutions.length);
  },

  pushMLResolution(state, res) {
    state.ml.resolutions.push(res);
  },

  setDLResolutions(state, { isPending, resolutions }) {
    state.dl.isPending = isPending;
    state.dl.resolutions = resolutions;
  },

  resetSAResolutions(state) {
    state.sa.pending = false;
    state.sa.nopatches = true;
    state.sa.timeout = false;
    state.sa.resolutions.splice(0, state.sa.resolutions.length);
    state.sa.merged = [];
    state.sa.patches.slice(0, state.sa.patches.length);
    // state.sa.patchStartLine = -1;
  },

  setSAResolutionPending(state, pending) {
    state.sa.pending = pending;
  },

  setSAResolutionNoPatches(state, nopatches) {
    state.sa.nopatches = nopatches;
  },

  setSAResolutionTimeout(state, timeout) {
    state.sa.timeout = timeout;
  },

  setSAResolutions(state, res) {
    state.sa.resolutions.splice(0, state.sa.resolutions.length);
    // state.sa.patches.splice(0, state.sa.patches.length);
    res.resolutions.forEach(reso => {
      state.sa.resolutions.push({
        desc: reso.desc,
        confidence: reso.confidence,
        index: reso.index + 1,
        label: reso.label,
        resolution: reso.code.join("\n"),
      });
    });

    if (!state.sa.nopatches) {
      state.sa.merged = res.merged;
      state.sa.patches = constructSemanticPatch(
        state.editor.rtModified,
        customGitDiff(
          state.editor.rtModified.join("\n"),
          state.sa.merged.join("\n"),
        ),
      );
      state.sa.constPatches = state.sa.patches;
    }
  },

  initResolveEditorContent(state, conflict) {
    const lines = conflict.split("\n");
    const conflictBlocks = findConflictBlocks(lines);

    const fingerPrints = conflictBlocks.map(block => {
      return getBlockHash(
        lines,
        block.ourMarkerLineNo - 1,
        block.endMarkerLineNo - 1,
      );
    });

    // use object assignment will lose reactivity
    state.editor.conflictBlocks = conflictBlocks;
    state.editor.versions = [];
    state.editor.currentVerIndex = -1;
    state.editor.originalFingerprints = fingerPrints;
    state.editor.locationMapping.clear();

    state.editor.original = lines;
    state.editor.modified = lines;
    state.editor.rtModified = lines;

    state.remainingConflictBlocks = conflictBlocks;
  },

  initFileParams(state, file) {
    state.file = file;
  },

  saveModifiedContent(state, payload) {
    const { newContent, line } = payload;
    // DOUBLE Buffering:
    // don't update the content in modified editor here: it's too heavy to react to minor changes
    state.editor.rtModified = newContent.split("\n");
    if (state.sa.patches.length) {
      state.sa.patches = constructSemanticPatch(
        state.editor.rtModified,
        customGitDiff(
          state.editor.rtModified.join("\n"),
          state.sa.merged.join("\n"),
          line,
        ),
      );
    }
  },

  storeLocationMapping(state, { payload, blockInterval }) {
    state.editor.locationMapping.set(payload, blockInterval);
  },

  applyResolution(state, payload) {
    const { index, type } = payload;

    if (type === "patch") {
      const patch = state.sa.patches[index];
      const payload = {
        index,
        patch,
      };
      this.commit("diff/applyPatch", payload);
    } else {
      const interval = state.editor.locationMapping.get(payload);
      if (!interval)
        throw new Error("未预料到的错误：无法找到需要替换的冲突块位置");

      const { start, end } = interval;

      if (type === "ours") {
        const block = state.editor.conflictBlocks[index];

        const ourStart = block.ourMarkerLineNo;
        const ourEnd =
          block.baseMarkerLineNo === -1
            ? block.theirMarkerLineNo
            : block.baseMarkerLineNo;
        const ourSlice = getArraySlice(
          state.editor.original,
          ourStart - 1 + 1,
          ourEnd - 1 - 1,
        );
        const payload = {
          start,
          end,
          resolution: ourSlice.join("\n"),
        };
        this.commit("diff/resolveConflictBlock", payload);
      } else if (type === "theirs") {
        const block = state.editor.conflictBlocks[index];

        const theirStart = block.theirMarkerLineNo;
        const theirEnd = block.endMarkerLineNo;
        const theirSlice = getArraySlice(
          state.editor.original,
          theirStart + 1 - 1,
          theirEnd - 1 - 1,
        );
        const payload = {
          start,
          end,
          resolution: theirSlice.join("\n"),
        };
        this.commit("diff/resolveConflictBlock", payload);
      } else if (type === "base") {
        const block = state.editor.conflictBlocks[index];

        const baseStart = block.baseMarkerLineNo;
        const baseEnd = block.theirMarkerLineNo;
        if (baseStart === -1 || baseEnd === -1)
          throw new Error("当前冲突区块无base版本解决方案");
        const baseSlice = getArraySlice(
          state.editor.original,
          baseStart + 1 - 1,
          baseEnd - 1 - 1,
        );
        const payload = {
          start,
          end,
          resolution: baseSlice.join("\n"),
        };
        this.commit("diff/resolveConflictBlock", payload);
      } else if (type === "sa") {
        if (state.sa.resolutions.length <= index)
          throw new Error("当前冲突区块无SA解决方案");

        const res = state.sa.resolutions[index];
        const payload = {
          start,
          end,
          resolution: res.resolution,
        };
        this.commit("diff/resolveConflictBlock", payload);
      } else if (type === "ml") {
        if (state.ml.resolutions.length <= index)
          throw new Error("当前冲突区块无ML解决方案");

        const res = state.ml.resolutions[index];
        const payload = {
          start,
          end,
          resolution: res.resolution,
        };
        this.commit("diff/resolveConflictBlock", payload);
      } else if (type === "git") {
        if (state.git.resolutions.length <= index)
          throw new Error("当前冲突区块无历史解决方案");

        const histories = state.git.resolutions[index];
        if (histories.length <= payload.historyIndex)
          throw new Error(
            `当前冲突区块无历史解决方案：${payload.historyIndex}`,
          );
        const res = histories[payload.historyIndex];
        const payload = {
          start,
          end,
          resolution: res.res,
        };
        this.commit("diff/resolveConflictBlock", payload);
      } else if (type === "dl") {
        if (state.dl.isPending) {
          // 弹窗提示
          Vue.prototype.$message.warning("DL解决方案正在获取中，请稍后再试");
          return;
        }
        if (state.dl.resolutions.length <= index)
          throw new Error("当前冲突区块无DL解决方案");
        if (state.dl.resolutions[index].resolvable === false) {
          Vue.prototype.$message.warning(
            "该冲突 DL 解决方案不可用，请选择其他解决方案或手动解决",
          );
          return;
        }
        let res = state.dl.resolutions[index];
        if (res.resolution[res.resolution.length - 1] === "\n") {
          res.resolution = res.resolution.slice(0, -1);
        }
        const payload = {
          start,
          end,
          resolution: res.resolution,
        };
        this.commit("diff/resolveConflictBlock", payload);
      } else {
        throw new Error(`未预料到的错误：未预料到的解决方案类型：${type}`);
      }
    }

    state.remainingConflictBlocks = findConflictBlocks(state.editor.rtModified);
  },

  resolveConflictBlock(state, payload) {
    const newContent = payload.resolution.split("\n");
    const lineNum = newContent.length;
    const newVer = state.editor.rtModified
      .slice(0, payload.start)
      .concat(newContent)
      .concat(state.editor.rtModified.slice(payload.end + 1)); // 0-based start and end

    state.patchStartLine = payload.start;
    this.commit("diff/saveVersion", state.editor.rtModified);
    this.commit(
      "diff/updateModifiedContent",
      newVer,
      payload.start + lineNum - 1,
    );
  },

  applyPatch(state, payload) {
    const { index, patch } = payload;
    const newContent = patch.newContent.split("\n");
    if (newContent[newContent.length - 1] === "") newContent.pop();
    const lineNum = newContent.length;
    // console.log(state.editor.rtModified.slice(0, patch.start - 1), newContent, state.editor.rtModified.slice(patch.start + patch.offset));
    const newVer = state.editor.rtModified
      .slice(0, patch.start - 1)
      .concat(newContent)
      .concat(state.editor.rtModified.slice(patch.start + patch.offset - 1)); // 1-based start
    state.patchStartLine = patch.start;
    this.commit("diff/saveVersion", state.editor.rtModified);
    this.commit("diff/updateModifiedContent", newVer, patch.start + lineNum);
  },

  saveVersion(state, oldVer) {
    const oldVerStr = oldVer.join("\n");
    state.editor.undoStack.push(oldVerStr);
    state.editor.redoStack.splice(0, state.editor.redoStack.length);
  },

  updateModifiedContent(state, lines, newline = -1) {
    // console.log(lines);
    state.editor.rtModified = lines;
    state.editor.modified = lines;

    if (state.sa.patches.length) {
      // rt diff
      state.sa.patches = constructSemanticPatch(
        state.editor.rtModified,
        customGitDiff(
          state.editor.rtModified.join("\n"),
          state.sa.merged.join("\n"),
          newline,
        ),
      );
    }
  },
  resetResolveEditor(state) {
    state.editor.rtModified = state.editor.original;
    state.editor.modified = state.editor.original;

    state.editor.undoStack.splice(0, state.editor.undoStack.length);
    state.editor.redoStack.splice(0, state.editor.redoStack.length);
    state.editor.locationMapping.clear();

    if (state.sa.merged && state.sa.merged.length) {
      state.sa.patches = constructSemanticPatch(
        state.editor.rtModified,
        customGitDiff(
          state.editor.rtModified.join("\n"),
          state.sa.merged.join("\n"),
        ),
      );
    }
    state.patchStartLine = -1;
    state.remainingConflictBlocks = findConflictBlocks(state.editor.rtModified);
  },

  refreshRemainingConflictBlocks(state) {
    state.remainingConflictBlocks = findConflictBlocks(state.editor.rtModified);
  },
};

export default {
  namespaced: true,
  state,
  getters,
  actions,
  mutations,
};
