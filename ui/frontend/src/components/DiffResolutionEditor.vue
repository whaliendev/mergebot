<script>
import {
  ref,
  onMounted,
  onBeforeUnmount,
  nextTick,
  onUnmounted,
  watch,
  computed,
} from "vue";
import * as monaco from "monaco-editor";
import { getConflictBlockDecorations } from "@/utils/merge";
import {
  extractHoverContents,
  addPatchInteractions,
  useAddContextMenuItems,
  hackOriginalContextMenu,
  SHOW_ORIGINAL_RECOMMENDATION,
  SHOW_MODIFIED_RECOMMENDATION,
  hackModifiedContextMenu,
} from "@/utils/ui";
import store from "@/store";

let editor = null;
const providers = [];
const modifiedProviders = [];
const menuActions = [];
const modifiedMenuActions = [];
let userTriggerred = false;
let showOriginalCondition;
let showModifiedCondition;

export default {
  props: {
    original: {
      type: String,
      default: 'int main() {\n    printf("Hello World!");\n    return 0;\n}',
    },
    modified: {
      type: String,
      default: 'int main() {\n    printf("Hello World!");\n    return 0;\n',
    },
    dir: {
      type: String,
      default: "v",
    },
    lang: {
      type: String,
      default: "cpp",
    },
  },
  emits: ["re-layout", "update:modified", "block-change"],
  setup(props, context) {
    const editorRef = ref(null);
    const editorWrapperRef = ref(null);
    const crowed = ref(false);

    const blockResolutions = computed(() => {
      // fuck vuex3
      return store.getters["diff/blockResolutions"];
    });

    const conflictBlocks = computed(() => {
      return store.state.diff.editor.conflictBlocks;
    });

    watch(conflictBlocks, (newVal, oldVal) => {
      if (editor && showOriginalCondition) {
        hackOriginalContextMenu(
          editor.getOriginalEditor(),
          newVal,
          showOriginalCondition,
        );
      }
    });

    const remainingCnt = computed(() => {
      return store.getters["diff/remainingConflictBlocksCnt"];
    });

    const index = ref(-1);
    const total = computed(() => {
      return conflictBlocks.value.length;
    });

    const canUndo = computed(() => {
      return store.getters["diff/canUndo"];
    });

    const canRedo = computed(() => {
      return store.getters["diff/canRedo"];
    });

    const patches = computed(() => {
      return store.getters["diff/patches"];
    });

    watch(patches, (newVal, oldVal) => {
      // console.log(`patch update at ${new Date()}`);
      addPatchInteractions(
        newVal,
        editor,
        modifiedProviders,
        props.lang,
        modifiedMenuActions,
      );
    });

    watch(patches, (newVal, oldVal) => {
      if (editor && showModifiedCondition) {
        hackModifiedContextMenu(
          editor.getModifiedEditor(),
          newVal,
          showModifiedCondition,
        );
      }
    });

    const decorateOriginalEditor = editor => {
      const newDecorations = getConflictBlockDecorations(conflictBlocks.value);
      // console.log(newDecorations);
      const originalEditor = editor.getOriginalEditor();
      originalEditor.deltaDecorations([], newDecorations);

      // provide hovers
      registerHoverProvider(conflictBlocks.value);

      const addContextMenuItems = useAddContextMenuItems(
        conflictBlocks.value,
        menuActions,
      );
      addContextMenuItems(originalEditor);
    };

    // extract to hooks.js
    // The reason why we didn't extract this part of hook from `DiffResolutionEditor` and `ReadonlyEditor` is
    // it seems that this extraction doesn't work with Vue 2.7.
    const reactToParentResize = () => {
      let timer = null;
      const resizeHandler = () => {
        if (timer) {
          return;
        }

        timer = setTimeout(() => {
          nextTick(() => {
            if (!editorWrapperRef.value) {
              return;
            }

            let { width, height } =
              editorWrapperRef.value.getBoundingClientRect();
            crowed.value = (props.dir === "h" ? width : height) < 100;
            context.emit("re-layout", crowed.value);
            editor && editor.layout();
            timer = null;
          });
        }, 30);
      };

      const resizeObserver = new ResizeObserver(entries => {
        for (const entry of entries) {
          if (entry.target.classList.contains("draggable-item")) {
            resizeHandler();
          }
        }
      });

      onMounted(() => {
        if (!editorWrapperRef.value || !editorWrapperRef.value.parentNode) {
          return;
        }
        resizeObserver.observe(editorWrapperRef.value.parentNode);

        decorateOriginalEditor(editor);
      });

      onUnmounted(() => {
        if (!editorWrapperRef.value || !editorWrapperRef.value.parentNode) {
          return;
        }
        resizeObserver.unobserve(editorWrapperRef.value.parentNode);
      });
    };

    // diff editor content related logic
    onMounted(() => {
      if (editorRef.value) {
        editor = monaco.editor.createDiffEditor(editorRef.value, {
          theme: "vs-light",
          renderSideBySide: true,
          experimental: {
            showMoves: true,
          },
          minimap: {
            enabled: true,
          },
          glyphMargin: true,
        });

        const originalModel = monaco.editor.createModel(
          props.original,
          props.lang,
        );
        const modifiedModel = monaco.editor.createModel(
          props.modified,
          props.lang,
        );
        editor.setModel({
          original: originalModel,
          modified: modifiedModel,
        });

        editor.getModifiedEditor().onDidChangeCursorSelection(ev => {
          if (ev.reason !== monaco.editor.CursorChangeReason.ContentFlush) {
            userTriggerred = true;
          }
        });

        editor.getModel().modified.onDidChangeContent(e => {
          // debugger;
          if (!userTriggerred) return;
          const payload = {
            newContent: editor.getModel().modified.getValue(),
            line: editor.getModifiedEditor().getPosition().lineNumber,
          };
          context.emit("update:modified", payload);
        });

        const originalEditor = editor.getOriginalEditor();
        showOriginalCondition = originalEditor.createContextKey(
          SHOW_ORIGINAL_RECOMMENDATION,
          false,
        );

        const modifiedEditor = editor.getModifiedEditor();
        showModifiedCondition = modifiedEditor.createContextKey(
          SHOW_MODIFIED_RECOMMENDATION,
          false,
        );

        watch(
          () => props.original,
          newVal => {
            if (editor) {
              editor.getOriginalEditor().setValue(newVal);

              decorateOriginalEditor(editor);
            }
          },
        );

        watch(
          () => props.modified,
          newVal => {
            if (editor) {
              // debugger;
              userTriggerred = false;
              editor.getModifiedEditor().setValue(newVal);

              addPatchInteractions(
                patches.value,
                editor,
                modifiedProviders,
                props.lang,
                modifiedMenuActions,
              );
            }
          },
        );

        watch(
          index,
          newVal => {
            if (conflictBlocks.value.length > newVal && newVal >= 0) {
              const originalEditor = editor.getOriginalEditor();

              const blockLineNo = conflictBlocks.value[newVal].ourMarkerLineNo;
              if (!blockLineNo) {
                return;
              }
              originalEditor.revealLineNearTop(
                blockLineNo,
                monaco.editor.ScrollType.Smooth,
              );
              const lineCnt = originalEditor.getModel().getLineCount();
              context.emit("block-change", {
                percentage: blockLineNo / lineCnt,
              });
            }
          },
          { immediate: true },
        );
      }
    });

    // clean up
    onBeforeUnmount(() => {
      if (editor) {
        editor.dispose();
      }
    });

    const registerHoverProvider = conflictBlocks => {
      const originalEditor = editor.getOriginalEditor();
      if (!originalEditor) {
        return;
      }

      providers.forEach(provider => {
        provider.dispose();
      });

      const hoverProvider = monaco.languages.registerHoverProvider("*", {
        // eslint-disable-next-line no-unused-vars
        provideHover: (model, position, token) => {
          if (model !== originalEditor.getModel()) {
            return null;
          }

          const line = position.lineNumber;
          const blockUnderCursorIndex = conflictBlocks.findIndex(block => {
            return (
              block.ourMarkerLineNo <= line && line <= block.endMarkerLineNo
            );
          });

          if (blockUnderCursorIndex < 0) {
            return null;
          }

          const blockUnderCursor = conflictBlocks[blockUnderCursorIndex];
          // eslint-disable-next-line no-unused-vars
          // const [historyResos, mlResos, saResos] = blockResolutions.value;
          const contents = extractHoverContents(
            blockResolutions.value,
            blockUnderCursorIndex,
            props.lang,
          );
          // console.log(contents);
          return {
            range: new monaco.Range(
              blockUnderCursor.ourMarkerLineNo,
              1,
              blockUnderCursor.endMarkerLineNo,
              80,
            ),
            contents,
          };
        },
      });

      providers.push(hoverProvider);
    };

    reactToParentResize();

    const changeConflictBlock = step => {
      if (
        index.value + step < 0 ||
        index.value + step >= conflictBlocks.value.length
      ) {
        return;
      }
      index.value += step;
      store.commit("diff/refreshRemainingConflictBlocks");
    };

    const handleUndoChanges = async () => {
      await store.dispatch("diff/undoChanges");
    };

    const handleRedoChanges = async () => {
      await store.dispatch("diff/redoChanges");
    };

    return {
      editorRef,
      editorWrapperRef,
      crowed,
      changeConflictBlock,
      index,
      total,
      canUndo,
      canRedo,
      handleUndoChanges,
      handleRedoChanges,
      remainingCnt,
    };
  },
};
</script>

<template>
  <div
    class="diff-resolution-wrapper h-full w-full flex flex-col overflow-hidden"
    ref="editorWrapperRef"
  >
    <div
      class="diff-resolution-header w-full h-12 px-1 bg-white border-b-neutral-200 border-b-2 overflow-hidden"
    >
      <div
        class="diff-resolution-controls-container overflow-hidden flex justify-between items-center w-full h-full"
      >
        <div class="status-container flex items-center space-around">
          还剩&nbsp;&nbsp;{{ remainingCnt }}&nbsp;&nbsp;个冲突块未解决
        </div>

        <div class="controls-container flex items-center justify-between mr-4">
          <div class="progress-container">
            <el-button
              icon="el-icon-arrow-up"
              @click="changeConflictBlock(-1)"
              size="small"
              >上一个冲突块</el-button
            >
            <el-button size="small">{{ index + 1 }}/{{ total }}</el-button>
            <el-button
              icon="el-icon-arrow-down"
              @click="changeConflictBlock(1)"
              size="small"
              >下一个冲突块</el-button
            >
          </div>

          <div class="undo-redo-container ml-5">
            <el-button
              icon="el-icon-refresh-left"
              size="small"
              title="回到上个版本"
              class="w-[50px]"
              @click="handleUndoChanges"
              :disabled="!canUndo"
            ></el-button>
            <el-button
              icon="el-icon-refresh-right"
              size="small"
              title="去往下个版本"
              class="w-[50px]"
              @click="handleRedoChanges"
              :disabled="!canRedo"
            ></el-button>
          </div>
        </div>
      </div>
    </div>
    <div class="diff-resolution-editor h-full w-full" ref="editorRef"></div>
  </div>
</template>

<style scoped lang="scss">
.diff-resolution-editor :deep(.editor) {
  .monaco-hover,
  .monaco-hover-content {
    max-width: 800px !important;
    max-height: 400px !important;
  }
}

.diff-resolution-controls-container .status-container {
  font-size: 14px;
  color: #666;
  margin-left: 0.75rem;
}
</style>

<style lang="scss">
.conflicts-ours {
  background: rgba(173, 216, 230, 0.8);
  width: 28px !important;
  margin-left: 6px;
  margin-right: 4px;
}

.conflicts-ours:after {
  content: "=>";
  color: #767676;
}

.conflicts-base {
  background: rgba(169, 169, 169, 0.4);
  width: 28px !important;
  margin-left: 6px;
  margin-right: 4px;

  &:after {
    content: "==";
    color: #767676;
  }
}

.conflicts-theirs {
  background: rgba(255, 192, 203, 0.8);
  width: 28px !important;
  margin-left: 6px;
  margin-right: 4px;

  &:after {
    content: "<=";
    color: #767676;
  }
}

.conflicts-block {
  background-color: rgba(220, 220, 220, 0.5);
}

.patch-glyph {
  background-color: #ffa07a;
  width: 28px !important;
  margin-left: 6px;
  margin-right: 4px;

  &::after {
    content: ">>";
    color: #767676;
  }
}

.placeholder-decoration {
  background-color: #767676;
  width: 28px !important;
  margin-left: 6px;
  margin-right: 4px;
}
</style>
