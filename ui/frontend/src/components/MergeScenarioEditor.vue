<script setup>
import { ref, watch } from "vue";
import DraggableContainer from "./DraggableContainer.vue";
import DraggableItem from "./DraggableItem.vue";
import ReadonlyEditor from "./ReadonlyEditor.vue";
import { throttle } from "@/utils";

const props = defineProps({
  revArr: {
    type: Array,
    default() {
      return [];
    },
  },
  revealRatio: {
    type: Number,
    default: 0,
  },
});

const scrollPayload = ref({});
const handleScrollChange = throttle(payload => {
  // console.log(payload);
  scrollPayload.value = payload;
}, 100);
</script>

<template>
  <draggable-container :number="3" dir="h" class="pr-1">
    <draggable-item
      v-for="(item, index) in revArr"
      :key="item.title"
      :index="index"
      :disabled="item.disabled"
      :item-title="item.title"
      v-slot="parent"
    >
      <readonly-editor
        :content="item.content"
        :lang="item.lang"
        :scroll-payload="scrollPayload"
        :close-handler="parent.handleEditorRequestClose"
        :item-title="parent.itemTitle"
        :reveal-ratio="revealRatio"
        @scroll-change="handleScrollChange"
      />
    </draggable-item>
    <!-- add a progress bar here -->
  </draggable-container>
</template>
