<script setup>
import { onBeforeUnmount, watch, inject, computed, provide } from "vue";
import Drag from "@/utils/drag.js";

const props = defineProps({
  disabled: {
    type: Boolean,
    default: false,
  },
  touchBarSize: {
    type: Number,
    default: 12,
  },
  index: {
    type: Number,
    default: 0,
  },
  showTouchBar: {
    type: Boolean,
    default: true,
  },
  hide: {
    type: Boolean,
    default: false,
  },
  itemTitle: {
    type: String,
    default: "",
  },
});

provide("index", props.index);

const emit = defineEmits(["size-change"]);

const useInit = () => {
  const dir = inject("dir");

  return {
    dir,
  };
};

const useSizeList = ({ emit }) => {
  const sizeList = inject("sizeList");

  watch(
    [
      () => {
        return sizeList.value.length > 0
          ? sizeList.value[props.index].width
          : 0;
      },
      () => {
        return sizeList.value.length > 0
          ? sizeList.value[props.index].height
          : 0;
      },
    ],
    () => {
      emit("size-change");
    },
  );

  return {
    sizeList,
  };
};

const useDrag = ({ props }) => {
  // const { proxy } = getCurrentInstance()
  const onDragStart = inject("onDragStart");
  const onDrag = inject("onDrag");
  // 拖动方法
  let drag = null;
  if (!props.disabled) {
    // console.log('index', props.index)
    drag = new Drag(
      (...args) => {
        onDragStart(...args);
      },
      (...args) => {
        onDrag(props.index, ...args);
      },
      // (...args) => {
      //   proxy.$eventEmitter.emit('dragOver', ...args)
      // }
    );
  }

  // 拖动条鼠标按下事件
  const onMousedown = e => {
    // proxy.$eventEmitter.emit('dragStart')
    drag && drag.onMousedown(e);
  };

  // 即将解除挂载
  onBeforeUnmount(() => {
    drag && drag.off();
  });

  return {
    onMousedown,
  };
};

const touchBarSizeStyle = computed(() => {
  return {
    width: dir.value === "h" ? props.touchBarSize + "px" : "100%",
    height: dir.value === "h" ? "100%" : props.touchBarSize + "px",
  };
});

// created部分
const { dir } = useInit();
const { sizeList } = useSizeList({ emit });
const { onMousedown } = useDrag({ props });

// TODO(hwa): extract 1% to tailwindcss min-width, min-height,
// hardcode 1% here limit the scability and reusability
const draggableItemSizeStyle = computed(() => {
  let width = sizeList.value[props.index].width;
  // if programmer intend to make one item to be 100% width, we should set it to 100%;
  if (width > 98) width = 100;
  // 1 for min width of touch bar, 3 to take all the remaining space
  width = width == 1 ? 1 : width < 3 ? 0 : width;
  let height = sizeList.value[props.index].height;
  if (height > 98) height = 100;
  height = height == 1 ? 1 : height < 3 ? 0 : height;
  // TODO(hwa): maybe we could persist the width and height.
  // By having the parent container pass an id and combine the index as a key
  return {
    width: sizeList.value.length > 0 ? width + "%" : 0,
    height: sizeList.value.length > 0 ? height + "%" : 0,
  };
});

const handleEditorRequestClose = index => {
  if (sizeList.value.length > index) {
    const dim = dir.value === "h" ? "width" : "height";
    const nonzeroItems = sizeList.value.filter(
      item => item[dim] > 0 && item[dim] != 1,
    );
    if (nonzeroItems.length === 1) {
      return;
    }

    // 1 for min width of touch bar
    sizeList.value.splice(index, 1, {
      ...sizeList.value[index],
      [dim]: 1,
    });

    const restSize = (100 - 1) / (nonzeroItems.length - 1);
    nonzeroItems.forEach(item => {
      if (item[dim] !== 0) {
        item[dim] = restSize;
      }
    });
  }
};
</script>

<template>
  <div
    class="draggable-item"
    :class="[dir, { hide: hide }]"
    :style="draggableItemSizeStyle"
  >
    <div
      v-if="showTouchBar"
      class="touch-bar flex justify-center items-center"
      :style="touchBarSizeStyle"
      :class="[{ draggable: !disabled }, dir]"
      @mousedown="onMousedown"
    >
      <svg
        xmlns="http://www.w3.org/2000/svg"
        viewBox="0 0 1024 1024"
        :class="{ hidden: disabled }"
      >
        <path
          fill="#606266"
          d="M176 511a56 56 0 1 0 112 0a56 56 0 1 0-112 0zm280 0a56 56 0 1 0 112 0a56 56 0 1 0-112 0zm280 0a56 56 0 1 0 112 0a56 56 0 1 0-112 0z"
        />
      </svg>
    </div>
    <slot
      :handleEditorRequestClose="handleEditorRequestClose"
      :item-title="itemTitle"
    ></slot>
  </div>
</template>

<style scoped lang="scss">
.draggable-item {
  display: flex;
  background-color: #f0f0f0;
  overflow: hidden;

  &.hide {
    display: none;
  }

  & svg {
    width: 16px;
    height: 16px;
  }

  &.h svg {
    transform: rotate(90deg);
  }

  &.v {
    flex-direction: column;

    &:first-child {
      padding-top: 6px;
    }
  }

  .touch-bar {
    flex-grow: 0;
    flex-shrink: 0;
    background-color: #f0f0f0;
    border-radius: 4px;

    &:hover,
    &:focus {
      & path {
        fill: white;
      }
    }

    &.draggable {
      &.v {
        cursor: row-resize;
      }

      &.h {
        cursor: col-resize;
      }

      &:hover {
        background-color: #007aff;
      }
    }

    &.h {
      height: 100%;

      .title {
        display: block;
        margin-left: 0px;
        margin-top: 5px;
        text-align: center;
      }
    }

    &.v {
      width: 100%;
    }

    .title {
      display: flex;
      align-items: center;
      color: var(--editor-header-title-color);
      font-size: 12px;
      margin-left: 5px;
    }
  }
}
</style>
