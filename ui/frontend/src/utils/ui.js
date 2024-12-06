import monaco from "monaco-editor";
import Vue from "vue";
import store from "@/store";
import { getPatchDecorations } from "@/utils/merge";

export const extractHoverContents = (
  [_, mlResos, saResos, esResos],
  blockUnderCursorIndex,
  lang,
) => {
  const contents = [];

  const hoverContentHook = store.getters["diff/hoverContentForBlock"];

  const ourContent = hoverContentHook(blockUnderCursorIndex, "our");
  // if (ourContent && ourContent.length) {
  contents.push({
    value: `
**目标分支内容**：

\`\`\`${lang}
${ourContent}
\`\`\`
    `,
  });
  // }
  const baseContent = hoverContentHook(blockUnderCursorIndex, "base");
  // if (baseContent && baseContent.length) {
  contents.push({
    value: `
**基础版本内容**：

\`\`\`${lang}
${baseContent}
\`\`\`
    `,
  });
  // }
  const theirContent = hoverContentHook(blockUnderCursorIndex, "their");
  // if (theirContent && theirContent.length) {
  contents.push({
    value: `
**源分支内容**：

\`\`\`${lang}
${theirContent}
\`\`\`
    `,
  });
  // }

  if (saResos) {
    saResos.forEach(reso => {
      // TODO(hwa): extract hover template to support more personalized hover hints
      if (reso.index === blockUnderCursorIndex + 1) {
        contents.push({
          value: `
**启发式规则**：

解决方式：${reso.desc}

置信度：${reso.confidence}

解决方案：
\`\`\`${lang}
${reso.resolution}
\`\`\`
    `,
        });
      }
    });
  }

  if (mlResos) {
    mlResos.forEach(reso => {
      if (reso.index === blockUnderCursorIndex + 1) {
        contents.push({
          value: `
**机器学习**：

预测结果：${reso.label}

置信度：${reso.confidence}

解决方案：
\`\`\`${lang}
${reso.resolution}
\`\`\`
    `,
        });
      }
    });
  }

  // esResolver
  esResos
    .filter(reso => reso.idx === blockUnderCursorIndex + 1)
    .forEach(reso => {
      if (reso.resolvable) {
        contents.push({
          value: `
**深度学习**：

解决方案：
\`\`\`${lang}
${reso.resolution}
\`\`\`
        `,
        });
      } else {
        contents.push({
          value: `
**深度学习**：

暂无法解决该冲突，请手动解决
        `,
        });
      }
    });

  //   historyResos.forEach(reso => {
  //     if (reso.index === blockUnderCursorIndex + 1) {
  //       const historyTruths = reso.histories;

  //       historyTruths.forEach((history, index) => {
  //         contents.push({
  //           value: `
  // **历史解决方案${index + 1}**：

  // 置信度：${history.confidence}

  // 解决方案：
  // \`\`\`${lang}
  // ${history.res}
  // \`\`\`
  //           `
  //         });
  //       });
  //     }
  //   });

  return contents;
};

const checkInConflictingArea = (editor, conflictBlocks) => {
  const pos = editor.getPosition();
  const line = pos.lineNumber;
  const blockUnderCursorIndex = conflictBlocks.findIndex(block => {
    return block.ourMarkerLineNo <= line && line <= block.endMarkerLineNo;
  });
  if (blockUnderCursorIndex < 0) {
    Vue.prototype.$message({
      type: "warning",
      message: "当前光标不在冲突区块内，请将光标移动到冲突块上",
    });
    return {
      status: false,
    };
  }
  return {
    status: true,
    blockUnderCursorIndex, // 0-based
  };
};

const checkAndApplyResolution = async applyPayload => {
  const applicable = await store.dispatch(
    "diff/testIsApplicable",
    applyPayload,
  );
  if (applicable) {
    try {
      // here the sequence of dispatch and commit is important,
      // as if we apply resolution first, patch will get lost
      await store.dispatch("diff/saveBlockResolutionChoice", applyPayload);
      store.commit("diff/applyResolution", applyPayload);
    } catch (e) {
      Vue.$prototype.$message({
        type: "error",
        message: e.message,
      });
    }
  } else {
    Vue.prototype.$message({
      type: "warning",
      message:
        "未找到应用当前解决方案的冲突区块，请考虑重置合并后文件回上一个版本",
    });
  }
};

export const SHOW_ORIGINAL_RECOMMENDATION = "showOriginalRecommendation";
export const SHOW_MODIFIED_RECOMMENDATION = "showModifiedRecommendation";

export const useAddContextMenuItems = (conflictBlocks, menuActions) => {
  // eslint-disable-next-line no-unused-vars
  const addContextMenuItems = editor => {
    menuActions.forEach(action => {
      action.dispose();
    });

    const ourAction = editor.addAction({
      id: "ours",
      label: "选择目标分支的修改",
      keybindings: [
        monaco.KeyMod.CtrlCmd | monaco.KeyMod.Shift | monaco.KeyCode.Digit1,
      ],
      precondition: SHOW_ORIGINAL_RECOMMENDATION,
      contextMenuGroupId: "1_modification",
      contextMenuOrder: 1,
      async run(editor) {
        // debugger;
        const { status, blockUnderCursorIndex } = checkInConflictingArea(
          editor,
          conflictBlocks,
        );
        if (!status) return;

        const applyPayload = {
          index: blockUnderCursorIndex, // resolution index, 0-based, conflict block idx
          type: "ours",
          fingerIndex: blockUnderCursorIndex,
        };
        await checkAndApplyResolution(applyPayload);
      },
    });
    menuActions.push(ourAction);

    const baseAction = editor.addAction({
      id: "base",
      label: "保留基础版本的修改",
      keybindings: [
        monaco.KeyMod.CtrlCmd | monaco.KeyMod.Shift | monaco.KeyCode.Digit2,
      ],
      precondition: SHOW_ORIGINAL_RECOMMENDATION,
      contextMenuGroupId: "1_modification",
      contextMenuOrder: 2,
      async run(editor) {
        const { status, blockUnderCursorIndex } = checkInConflictingArea(
          editor,
          conflictBlocks,
        );
        if (!status) return;

        const applyPayload = {
          index: blockUnderCursorIndex, // resolution index, 0-based, conflict block idx
          type: "base",
          fingerIndex: blockUnderCursorIndex,
        };
        await checkAndApplyResolution(applyPayload);
      },
    });
    menuActions.push(baseAction);

    const theirAction = editor.addAction({
      id: "theirs",
      label: "选择源分支的修改",
      keybindings: [
        monaco.KeyMod.CtrlCmd | monaco.KeyMod.Shift | monaco.KeyCode.Digit3,
      ],
      precondition: SHOW_ORIGINAL_RECOMMENDATION,
      contextMenuGroupId: "1_modification",
      contextMenuOrder: 3,
      async run(editor) {
        const { status, blockUnderCursorIndex } = checkInConflictingArea(
          editor,
          conflictBlocks,
        );
        if (!status) return;

        const applyPayload = {
          index: blockUnderCursorIndex, // resolution index, 0-based, conflict block idx
          type: "theirs",
          fingerIndex: blockUnderCursorIndex,
        };
        await checkAndApplyResolution(applyPayload);
      },
    });
    menuActions.push(theirAction);

    const action1 = editor.addAction({
      id: "block-based-sa",
      label: "应用启发式方法推荐方案",
      keybindings: [
        monaco.KeyMod.CtrlCmd | monaco.KeyMod.Shift | monaco.KeyCode.Digit4,
      ],
      precondition: SHOW_ORIGINAL_RECOMMENDATION,
      contextMenuGroupId: "1_modification",
      contextMenuOrder: 4,
      async run(editor) {
        // debugger;
        const { status, blockUnderCursorIndex } = checkInConflictingArea(
          editor,
          conflictBlocks,
        );
        if (!status) return;

        // console.log(blockUnderCursorIndex);
        const index = store.state.diff.sa.resolutions.findIndex(reso => {
          return reso.index === blockUnderCursorIndex + 1;
        });
        if (index < 0) {
          Vue.prototype.$message({
            type: "warning",
            message: "未找到针对当前冲突区块的启发式解决方案",
          });
          return;
        } else {
          const applyPayload = {
            index, // resolution index, 0-based, here index means index in resolutions array
            type: "sa",
            fingerIndex: blockUnderCursorIndex,
          };
          await checkAndApplyResolution(applyPayload);
        }
      },
    });
    menuActions.push(action1);

    const actions2 = editor.addAction({
      id: "ml",
      label: "应用机器学习推荐方案",
      keybindings: [
        monaco.KeyMod.CtrlCmd | monaco.KeyMod.Shift | monaco.KeyCode.Digit5,
      ],
      precondition: SHOW_ORIGINAL_RECOMMENDATION,
      contextMenuGroupId: "1_modification",
      contextMenuOrder: 5,
      // eslint-disable-next-line no-unused-vars
      async run(editor) {
        const { status, blockUnderCursorIndex } = checkInConflictingArea(
          editor,
          conflictBlocks,
        );
        if (!status) return;

        const index = store.state.diff.ml.resolutions.findIndex(reso => {
          return reso.index === blockUnderCursorIndex + 1;
        });

        if (index < 0) {
          Vue.prototype.$message({
            type: "warning",
            message: "未找到针对当前冲突区块的机器学习解决方案",
          });
          return;
        } else {
          const applyPayload = {
            index, // resolution index, 0-based, index in resolutions array
            type: "ml",
            fingerIndex: blockUnderCursorIndex,
          };
          await checkAndApplyResolution(applyPayload);
        }
      },
    });
    menuActions.push(actions2);

    const action3 = editor.addAction({
      id: "dl",
      label: "应用深度学习推荐方案",
      keybindings: [
        monaco.KeyMod.CtrlCmd | monaco.KeyMod.Shift | monaco.KeyCode.Digit6,
      ],
      precondition: SHOW_ORIGINAL_RECOMMENDATION,
      contextMenuGroupId: "1_modification",
      contextMenuOrder: 6,
      async run(editor) {
        const { status, blockUnderCursorIndex } = checkInConflictingArea(
          editor,
          conflictBlocks,
        );
        if (!status) return;

        const applyPayload = {
          index: blockUnderCursorIndex, // resolution index, 0-based, block idx
          type: "dl",
          fingerIndex: blockUnderCursorIndex,
        };
        await checkAndApplyResolution(applyPayload);
      },
    });
    menuActions.push(action3);

    // const actions3 = editor.addAction({
    //   id: 'history-1',
    //   label: '应用历史解决方案1',
    //   keybindings: [
    //     monaco.KeyMod.CtrlCmd | monaco.KeyMod.Shift | monaco.KeyCode.Digit6,
    //   ],
    //   contextMenuGroupId: '1_modification',
    //   contextMenuOrder: 6,
    //   // eslint-disable-next-line no-unused-vars
    //   async run(editor) {
    //     const { status, blockUnderCursorIndex } = checkInConflictingArea(editor, conflictBlocks);
    //     if (!status) return;

    //     const index = store.state.diff.git.resolutions.findIndex(reso => {
    //       return reso.index === blockUnderCursorIndex + 1;
    //     });

    //     if (index < 0) {
    //       Vue.prototype.$message({
    //         type: 'warning',
    //         message: '未找到针对当前冲突区块的历史解决方案1'
    //       });
    //       return;
    //     } else {
    //       const applyPayload = {
    //         index, // resolution index, 0-based
    //         type: 'git',
    //         historyIndex: 0,
    //         fingerIndex: blockUnderCursorIndex
    //       };
    //       await checkAndApplyResolution(applyPayload);
    //     }
    //   }
    // });
    // menuActions.push(actions3);

    // const actions4 = editor.addAction({
    //   id: 'history-2',
    //   label: '应用历史解决方案2',
    //   keybindings: [
    //     monaco.KeyMod.CtrlCmd | monaco.KeyMod.Shift | monaco.KeyCode.Digit7,
    //   ],
    //   contextMenuGroupId: '1_modification',
    //   contextMenuOrder: 7,
    //   // eslint-disable-next-line no-unused-vars
    //   async run(editor) {
    //     const { status, blockUnderCursorIndex } = checkInConflictingArea(editor, conflictBlocks);
    //     if (!status) return;

    //     const index = store.state.diff.git.resolutions.findIndex(reso => {
    //       return reso.index === blockUnderCursorIndex + 1;
    //     });
    //     if (index < 0) {
    //       Vue.prototype.$message({
    //         type: 'warning',
    //         message: '未找到针对当前冲突区块的历史解决方案2'
    //       });
    //       return;
    //     } else {
    //       const applyPayload = {
    //         index, // resolution index, 0-based
    //         type: 'git',
    //         historyIndex: 1,
    //         fingerIndex: blockUnderCursorIndex
    //       };
    //       await checkAndApplyResolution(applyPayload);
    //     }
    //   }
    // });
    // menuActions.push(actions4);

    // const actions5 = editor.addAction({
    //   id: 'history-3',
    //   label: '应用历史解决方案3',
    //   keybindings: [
    //     monaco.KeyMod.CtrlCmd | monaco.KeyMod.Shift | monaco.KeyCode.Digit8,
    //   ],
    //   contextMenuGroupId: '1_modification',
    //   contextMenuOrder: 8,
    //   // eslint-disable-next-line no-unused-vars
    //   async run(editor) {
    //     const { status, blockUnderCursorIndex } = checkInConflictingArea(editor, conflictBlocks);
    //     if (!status) return;

    //     const index = store.state.diff.git.resolutions.findIndex(reso => {
    //       return reso.index === blockUnderCursorIndex + 1;
    //     });

    //     if (index < 0) {
    //       Vue.prototype.$message({
    //         type: 'warning',
    //         message: '未找到针对当前冲突区块的历史解决方案3'
    //       });
    //       return;
    //     } else {
    //       const applyPayload = {
    //         index, // resolution index, 0-based
    //         type: 'git',
    //         historyIndex: 2,
    //         fingerIndex: blockUnderCursorIndex
    //       };
    //       await checkAndApplyResolution(applyPayload);
    //     }
    //   }
    // });
    // menuActions.push(actions5);
  };
  return addContextMenuItems;
};

const checkInPatchableZone = (editor, patches) => {
  const pos = editor.getPosition();
  const line = pos.lineNumber;
  const patchUnderCursorIndex = patches.findIndex(patch => {
    return patch.start <= line && line <= patch.start + patch.offset - 1;
  });
  if (patchUnderCursorIndex === -1) {
    Vue.prototype.$message({
      type: "warning",
      message: "当前光标不在可应用补丁区域，请将光标移动到可应用补丁区域",
    });
    return {
      status: false,
    };
  }
  return {
    status: true,
    patchUnderCursorIndex,
  };
};

export const useAddContextMenuItemsToModified = (patches, menuActions) => {
  const addContextMenuItems = editor => {
    menuActions.forEach(action => {
      action.dispose();
    });

    const ourAction = editor.addAction({
      id: "semantic",
      label: "应用语义补丁",
      keybindings: [
        monaco.KeyMod.CtrlCmd | monaco.KeyMod.Alt | monaco.KeyCode.Digit1,
      ],
      precondition: SHOW_MODIFIED_RECOMMENDATION,
      contextMenuGroupId: "1_modification",
      contextMenuOrder: 1,
      async run(editor) {
        const { status, patchUnderCursorIndex } = checkInPatchableZone(
          editor,
          patches,
        );
        if (!status) return;

        const applyPayload = {
          index: patchUnderCursorIndex, // in resolutions array index
          type: "patch",
          fingerIndex: patchUnderCursorIndex,
        };
        await checkAndApplyResolution(applyPayload);
      },
    });
    menuActions.push(ourAction);
  };

  return addContextMenuItems;
};

export const addPatchInteractions = (
  patches,
  editor,
  modifiedProviders,
  lang,
  modifiedMenuActions,
) => {
  if (!patches.length) return;
  if (editor) {
    const modifiedEditor = editor.getModifiedEditor();

    const prevDecorations = modifiedEditor.getModel().getAllDecorations();
    const prevIds = prevDecorations.map(decoration => decoration.id);
    modifiedEditor.removeDecorations(prevIds);
    const patchDecorations = getPatchDecorations(patches);
    modifiedEditor.deltaDecorations([], patchDecorations);

    const addContextMenuItemToModified = useAddContextMenuItemsToModified(
      patches,
      modifiedMenuActions,
    );
    addContextMenuItemToModified(modifiedEditor);
    // console.log(patchDecorations);

    modifiedProviders.forEach(provider => {
      provider.dispose();
    });

    const hoverProvider = monaco.languages.registerHoverProvider(lang, {
      provideHover: (model, position, token) => {
        if (model !== modifiedEditor.getModel()) {
          return null;
        }

        const line = position.lineNumber;
        const patch = patches.find(patch => {
          return patch.start <= line && line <= patch.start + patch.offset - 1;
        });

        if (!patch) {
          return null;
        }

        const contents = [
          {
            value: `
**语义补丁：**
\`\`\`${lang}
${patch.newContent}
\`\`\`
          `,
          },
        ];

        return {
          range: new monaco.Range(
            patch.start,
            1,
            patch.start + patch.offset - 1,
            80,
          ),
          contents,
        };
      },
    });

    modifiedProviders.push(hoverProvider);
  }
};

export const hackOriginalContextMenu = (
  originalEditor,
  conflictBlocks,
  showCondition,
) => {
  const originalContextMenu = originalEditor.getContribution(
    "editor.contrib.contextmenu",
  );
  const originalRealMethod = originalContextMenu._onContextMenu;
  originalContextMenu._onContextMenu = function (...args) {
    if (args.length === 0) {
      return;
    }
    const contextMenuObj = args[0];
    const target = contextMenuObj.target;
    const pos = target.position;
    const line = pos.lineNumber;
    const blockUnderCursorIndex = conflictBlocks.findIndex(block => {
      return block.ourMarkerLineNo <= line && line <= block.endMarkerLineNo;
    });

    showCondition.set(!(blockUnderCursorIndex < 0));
    originalRealMethod.call(originalContextMenu, ...args);
  };
};

export const hackModifiedContextMenu = (
  modifiedEditor,
  patches,
  showCondition,
) => {
  const modifiedContextMenu = modifiedEditor.getContribution(
    "editor.contrib.contextmenu",
  );
  const modifiedRealMethod = modifiedContextMenu._onContextMenu;
  modifiedContextMenu._onContextMenu = function (...args) {
    if (args.length === 0) {
      return;
    }
    const contextMenuObj = args[0];
    const target = contextMenuObj.target;
    const pos = target.position;
    const line = pos.lineNumber;
    // console.log(patches);
    const patchUnderCursorIndex = patches.findIndex(patch => {
      return patch.start <= line && line <= patch.start + patch.offset - 1;
    });

    showCondition.set(!(patchUnderCursorIndex < 0));
    modifiedRealMethod.call(modifiedContextMenu, ...args);
  };
};
