<script setup>
import {
  ref,
  onMounted,
  onBeforeUnmount,
  nextTick,
  inject,
  watch,
  computed,
} from "vue";
import * as monaco from "monaco-editor";
import { throttle } from "@/utils";

const props = defineProps({
  content: {
    type: String,
    default: "",
  },
  lang: {
    type: String,
    default: "cpp",
  },
  dir: {
    type: String,
    default: "h",
  },
  scrollPayload: {
    type: Object,
    default() {
      return {};
    },
  },
  revealRatio: {
    type: Number,
    default: 0,
  },
  closeHandler: {
    type: Function,
    default: () => {},
  },
  itemTitle: {
    type: String,
    default: "",
  },
});

const index = inject("index");

const emit = defineEmits(["re-layout", "scroll-change"]);

const editorWrapperRef = ref(null);
const editorRef = ref(null);
const crowed = ref(false);
let editor = null;

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

        let { width, height } = editorWrapperRef.value.getBoundingClientRect();
        // console.log(width, height);
        crowed.value = (props.dir === "h" ? width : height) < 100;
        emit("re-layout", crowed.value);
        editor && editor.layout();
        timer = null;
      });
    }, 30);
  };

  const resizeObserver = new ResizeObserver(entries => {
    for (const entry of entries) {
      // console.log(entry);
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
  });

  onBeforeUnmount(() => {
    if (!editorWrapperRef.value || !editorWrapperRef.value.parentNode) {
      return;
    }
    resizeObserver.unobserve(editorWrapperRef.value.parentNode);
  });
};

const programmaticScroll = ref(false);
onMounted(() => {
  if (editorRef.value) {
    editor = monaco.editor.create(editorRef.value, {
      value: props.content,
      language: props.lang,
      theme: "vs-light",
      readOnly: true,
      // alwaysConsumeMouseWheel: false,
    });

    editor.onDidScrollChange(
      throttle(e => {
        if (programmaticScroll.value) {
          programmaticScroll.value = false;
          return;
        }

        const editorDomEl = editor.getDomNode();
        if (!editorDomEl) return;
        const visibleHeight = editorDomEl.offsetHeight;
        // console.log(e.scrollTop, e.scrollHeight, visibleHeight)

        const scrollPercentage = e.scrollTop / (e.scrollHeight - visibleHeight);

        emit("scroll-change", {
          index,
          percentage: scrollPercentage,
        });
      }, 100),
    );
  }
});

watch(
  () => props.scrollPayload.percentage,
  newVal => {
    if (props.scrollPayload.index === index) return;
    if (editor) {
      const editorDomEl = editor.getDomNode();
      if (!editorDomEl) return;
      programmaticScroll.value = true;
      // console.log(newVal);
      const visibleHeight = editorDomEl.offsetHeight;
      const contentHeight = editor.getContentHeight();

      const newScrollTop = (contentHeight - visibleHeight) * newVal;

      editor.setScrollTop(newScrollTop);
    }
  },
);

watch(
  () => props.revealRatio,
  newVal => {
    if (!editor) return;

    const totalLines = editor.getModel().getLineCount();
    const revealLine = Math.floor(totalLines * newVal);
    // console.log(revealLine);
    editor.revealLineNearTop(revealLine, monaco.editor.ScrollType.Smooth);
  },
);

onBeforeUnmount(() => {
  if (editor) {
    editor.dispose();
  }
});

const handleEditorClose = () => {
  props.closeHandler(index);
};

reactToParentResize();

watch(
  () => props.content,
  newVal => {
    if (editor) {
      editor.setValue(newVal);
    }
  },
);

const title = computed(() => {
  // console.log(props.itemTitle);
  if (props.itemTitle && props.itemTitle.length === 40) {
    return props.itemTitle.substr(0, 8);
  }
  return props.itemTitle;
});
</script>

<template>
  <div
    class="readonly-editor-wrapper h-full w-full flex flex-col rounded-lg overflow-hidden border-[1px] border-neutral-200"
    ref="editorWrapperRef"
  >
    <div
      class="readonly-editor-header w-full h-7 flex justify-between items-center pl-4 pr-2 grow-0 shrink-0 bg-white border-b-neutral-200 border-b-2"
    >
      <div class="meta-info-container">
        {{ title }}
      </div>

      <div class="controls-container">
        <el-button
          icon="el-icon-arrow-down"
          size="mini"
          @click="handleEditorClose"
        >
        </el-button>
      </div>
    </div>
    <div class="readonly-editor-container w-full h-full" ref="editorRef"></div>
  </div>
</template>

<style scoped lang="scss">
.readonly-editor-header {
  .meta-info-container {
    font-size: 14px;
    color: #555;
  }

  .controls-container {
    height: 100%;
    overflow: hidden;

    :deep(.el-button) {
      border: 0;
      width: 100%;
      height: 100%;

      &:hover,
      &:focus {
        background-color: #f0f0f0;
        color: #333;
      }

      .el-icon-arrow-down {
        font-size: 16px;
        font-weight: 500;
      }
    }
  }
}
</style>
