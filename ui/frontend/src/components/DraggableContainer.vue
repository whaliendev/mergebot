<script setup>
import { ref, onMounted, provide, onBeforeUnmount } from "vue";
import Resize from "@/utils/resize";
import ResizeObserver from "resize-observer-polyfill";

const props = defineProps({
  // draggable direction
  dir: {
    type: String,
    default: "h",
  },
  // num of internal DragItems
  number: {
    type: Number,
    default: 0,
  },
  // configs
  config: {
    type: Array,
    default() {
      return [];
    },
  },
});

const containerRef = ref(null);

const useInitResize = ({ props }) => {
  const resize = new Resize();
  const { onDragStart, onDrag } = resize;
  provide("onDragStart", onDragStart);
  provide("onDrag", onDrag);
  provide("dir", ref(props.dir));

  return {
    resize,
  };
};

const ureInitSizeList = ({ props }) => {
  let sizeListData = [];
  for (let i = 0; i < props.number; ++i) {
    sizeListData.push({
      width: 0,
      height: 0,
      ...{
        default: props.config[i]?.default ? props.config[i].default : undefined,
        width: props.config[i]?.width ? props.config[i].width : 0,
        height: props.config[i]?.height ? props.config[i].height : 0,
        min: props.config[i]?.min ? props.config[i].min : 20,
      },
    });
  }

  // console.log('sizeListData', sizeListData)

  const sizeList = ref(sizeListData);
  provide("sizeList", sizeList);

  return {
    sizeList,
  };
};

const useInitSize = ({ sizeList, props }) => {
  const first = ref(true);
  const setInitSize = (width, height) => {
    if (!first.value) return;

    first.value = false;

    let totalSetDefaultSize = 0;
    let totalSetDefaultsNum = 0;
    sizeList.value.forEach(item => {
      if (item.default !== undefined) {
        totalSetDefaultSize += item.default;
        totalSetDefaultsNum++;
      }
    });

    // console.log('totalSetDefaultNum', totalSetDefaultsNum);
    // console.log('totalSetDefaultSize', totalSetDefaultSize);

    // distribute the rest of the space evenly
    const rest =
      100 - (totalSetDefaultSize / (props.dir === "h" ? width : height)) * 100;
    const restSize = rest / (props.number - totalSetDefaultsNum);
    // console.log('info', width, height, rest, restSize)
    sizeList.value.forEach(item => {
      if (props.dir == "h") {
        if (item.default === undefined) {
          item.width = restSize;
        } else {
          item.width = (item.default / width) * 100;
        }
        // console.log('horizontal item width: ', item.width)
        item.height = 100;
      } else {
        if (item.default === undefined) {
          item.height = restSize;
        } else {
          item.height = (item.default / height) * 100;
        }
        // console.log('vertical item height: ', item.height)
        item.width = 100;
      }
    });

    // console.log('calc:', sizeList);
  };

  return {
    setInitSize,
  };
};

/**
 *
 * @param setInitSize function to calculate the initial size of each DragItem
 * @param resize Resize instance
 * @param props props
 * @param sizeList sizeList
 */
const useResizeInit = ({ setInitSize, resize, props, sizeList }) => {
  const containerWidth = ref(0);
  const containerHeight = ref(0);
  const resizeInit = () => {
    const { width, height } = containerRef.value.getBoundingClientRect();
    // console.log('container width: ', width);
    // console.log('container height: ', height);
    containerWidth.value = width;
    containerHeight.value = height;
    setInitSize(width, height);
    resize.init({
      dir: props.dir,
      dragItemList: sizeList.value,
      containerSize: props.dir == "h" ? width : height,
    });
  };

  return {
    resizeInit,
  };
};

const useResizeObserver = ({ resizeInit }) => {
  const resizeObserver = new ResizeObserver(entries => {
    entries.forEach(entry => {
      entry.target.classList.contains("draggable-container") && resizeInit();
    });
  });

  onMounted(() => {
    resizeObserver.observe(containerRef.value);
  });

  onBeforeUnmount(() => {
    resizeObserver.unobserve(containerRef.value);
  });
};

const { resize } = useInitResize({ props });
const { sizeList } = ureInitSizeList({ props });
const { setInitSize } = useInitSize({ sizeList, props });
const { resizeInit } = useResizeInit({ setInitSize, resize, props, sizeList });
useResizeObserver({ resizeInit });
</script>

<template>
  <div class="draggable-container" :class="[dir]" ref="containerRef">
    <slot></slot>
  </div>
</template>

<style scoped>
.draggable-container {
  width: 100%;
  height: 100%;
  display: flex;
  flex-shrink: 1;
  flex-grow: 1;
  overflow: hidden;
}

.draggable-container.v {
  flex-direction: column;
}
</style>
