<script setup>
import {
  onBeforeMount,
  onBeforeUnmount,
  onMounted,
  ref,
  watch,
  nextTick,
} from "vue";
import DraggableContainer from "./DraggableContainer.vue";
import DraggableItem from "./DraggableItem.vue";
import * as monaco from "monaco-editor";
import store from "@/store";

const props = defineProps({
  revArr: {
    type: Array,
    default() {
      return [
        {
          title: "目标分支版本",
          content: "",
          lang: "java",
          disabled: false,
        },
        {
          title: "源分支版本",
          content: "",
          lang: "java",
          disabled: false,
        },
      ];
    },
  },
});

const targetEditorRef = ref(null);
const targetEditorWrapperRef = ref(null);

const sourceEditorRef = ref(null);
const sourceEditorWrapperRef = ref(null);
let targetEditor = null;
let sourceEditor = null;

onMounted(() => {
  // console.log(props);
  if (targetEditorRef.value) {
    targetEditor = monaco.editor.create(targetEditorRef.value, {
      value: props.revArr[0].content,
      lang: props.revArr[0].lang,
      theme: "vs-light",
      readOnly: true,
    });

    sourceEditor = monaco.editor.create(sourceEditorRef.value, {
      value: props.revArr[1].content,
      lang: props.revArr[1].lang,
      theme: "vs-light",
      readOnly: true,
    });
  }
});

onBeforeMount(() => {
  if (targetEditor) {
    targetEditor.dispose();
  }
  if (sourceEditor) {
    sourceEditor.dispose();
  }
});

watch(
  () => props.revArr[0].content,
  newVal => {
    if (targetEditor) {
      // console.log(newVal);
      targetEditor.setValue(newVal);
    }
  },
);

watch(
  () => props.revArr[1].content,
  newVal => {
    if (sourceEditor) {
      sourceEditor.setValue(newVal);
    }
  },
);

const reactToParentResize = () => {
  let timer = null;
  const resizeHandler = () => {
    if (timer) {
      return;
    }

    timer = setTimeout(() => {
      nextTick(() => {
        // any parent node is null, return
        if (!sourceEditorWrapperRef.value || !targetEditorWrapperRef.value) {
          return;
        }

        // let { width, height } = sourceEditorWrapperRef.value.getBoundingClientRect();
        // console.log(width, height);
        // crowed.value = (props.dir === "h" ? width : height) < 100;
        // emit("re-layout", crowed.value);
        sourceEditor && sourceEditor.layout();
        targetEditor && targetEditor.layout();
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
    if (
      !targetEditorWrapperRef.value ||
      !targetEditorWrapperRef.value.parentNode
    ) {
      return;
    }
    resizeObserver.observe(targetEditorWrapperRef.value.parentNode);
  });

  onBeforeUnmount(() => {
    if (
      !targetEditorWrapperRef.value ||
      !targetEditorWrapperRef.value.parentNode
    ) {
      return;
    }
    resizeObserver.unobserve(targetEditorWrapperRef.value.parentNode);
  });
};

reactToParentResize();

// react to refactoring idx change
let leftDecorations = [];
let rightDecorations = [];

const showRefactoring = newVal => {
  const leftRanges = newVal.leftSideLocations.map(location => ({
    range: new monaco.Range(
      location.startLine,
      location.startColumn,
      location.endLine,
      location.endColumn,
    ),
    options: {
      className: "refactor-highlight-left",
    },
  }));

  const rightRanges = newVal.rightSideLocations.map(location => ({
    range: new monaco.Range(
      location.startLine,
      location.startColumn,
      location.endLine,
      location.endColumn,
    ),
    options: {
      className: "refactor-highlight-right",
    },
  }));

  if (!targetEditor || !sourceEditor) {
    return;
  }

  // 清除旧高亮并添加新高亮
  leftDecorations = targetEditor.deltaDecorations(leftDecorations, leftRanges);
  rightDecorations = sourceEditor.deltaDecorations(
    rightDecorations,
    rightRanges,
  );

  // 计算最小行号并滚动到该行
  const minLeftLine = Math.min(
    ...newVal.leftSideLocations.map(loc => loc.startLine),
  );
  const minRightLine = Math.min(
    ...newVal.rightSideLocations.map(loc => loc.startLine),
  );

  if (targetEditor && minLeftLine) {
    targetEditor.revealLine(minLeftLine);
  }
  if (sourceEditor && minRightLine) {
    sourceEditor.revealLine(minRightLine);
  }
};

watch(
  () => store.getters["refactoring/refactoringToShow"],
  newVal => {
    if (newVal) {
      showRefactoring(newVal);
    }
  },
);
</script>

<template>
  <draggable-container :number="2" dir="h" class="pr-1">
    <!-- FIXME(hwa): we'd better use v-for and extract the duplicated code. -->
    <draggable-item :index="0" :disabled="false" :item-title="revArr[0].title">
      <div
        class="readonly-editor-wrapper h-full w-full flex flex-col rounded-lg overflow-hidden border-[1px] border-neutral-200"
        ref="targetEditorWrapperRef"
      >
        <div
          class="readonly-editor-header w-full h-7 flex justify-between items-center pl-4 pr-2 grow-0 shrink-0 bg-white border-b-neutral-200 border-b-2"
        >
          <div class="meta-info-container">
            {{ revArr[0].title }}
          </div>
        </div>
        <div
          class="readonly-editor-container w-full h-full"
          ref="targetEditorRef"
        ></div>
      </div>
    </draggable-item>
    <draggable-item :index="1" :disabled="false" :item-title="revArr[1].title">
      <div
        class="readonly-editor-wrapper h-full w-full flex flex-col rounded-lg overflow-hidden border-[1px] border-neutral-200"
        ref="sourceEditorWrapperRef"
      >
        <div
          class="readonly-editor-header w-full h-7 flex justify-between items-center pl-4 pr-2 grow-0 shrink-0 bg-white border-b-neutral-200 border-b-2"
        >
          <div class="meta-info-container">
            {{ revArr[1].title }}
          </div>
        </div>
        <div
          class="readonly-editor-container w-full h-full"
          ref="sourceEditorRef"
        ></div>
      </div>
    </draggable-item>
  </draggable-container>
</template>

<style scoped lang="scss">
.readonly-editor-header {
  .meta-info-container {
    font-size: 14px;
    color: #555;
  }
}
</style>

<style lang="scss">
.refactor-highlight-left {
  background-color: rgba(173, 216, 230, 0.8);
}

.refactor-highlight-right {
  background-color: rgba(255, 192, 203, 0.8);
}
</style>
