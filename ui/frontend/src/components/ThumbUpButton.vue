<template>
  <button class="button" :class="{ liked: liked }" @click="emitLikeRequest">
    <div class="hand">
      <div class="thumb"></div>
    </div>
    <span class="word">Like</span>
  </button>
</template>

<script setup>
import { ref } from "vue";

const { watch } = require("vue");

const props = defineProps({
  liked: Boolean,
});

const liked = ref(props.liked);

const emits = defineEmits(["like-requested"]);

const emitLikeRequest = () => {
  liked.value = !liked.value;
  emits("like-requested", liked.value);
};

// setLikeRequestedResult is exposed to parent component to reflect the liked state
const setLikeRequestedResult = result => {
  liked.value = result;
};
</script>

<style lang="scss" scoped>
.button {
  --color: #1e2235;
  --color-hover: #1e2235;
  --color-active: #fff;
  --icon: #bbc1e1;
  --icon-hover: #8a91b4;
  --icon-active: #fff;
  --background: #fff;
  --background-hover: #fff;
  --background-active: #fd7b7b;
  --border: #e1e6f9;
  --border-active: #fd7b7b;
  --shadow: #{rgba(#fd7b7b, 0.025)};

  display: inline-block;
  min-width: 96px;
  outline: none;
  cursor: pointer;
  position: relative;
  border: 0;
  background: none;
  padding: 4px 0;
  border-radius: 4px;
  line-height: 26px;
  font-family: inherit;
  font-weight: 400;
  font-size: 13px;
  color: var(--color);
  appearance: none;
  -webkit-tap-highlight-color: transparent;
  transition: color 0.2s linear;

  &:hover {
    --icon: var(--icon-hover);
    --color: var(--color-hover);
    --background: var(--background-hover);
    --border-width: 2px;
  }

  &:active {
    --scale: 0.95;
  }

  &:not(.liked) {
    &:hover {
      --hand-rotate: 8;
      --hand-thumb-1: -12deg;
      --hand-thumb-2: 36deg;

      .word {
        color: #f79e9e;
      }
    }
  }

  &.liked {
    --span-x: 2px;
    --span-d-o: 1;
    --span-d-x: 0;
    --icon: var(--icon-active);
    --color: var(--color-active);
    --border: var(--border-active);
    --background: var(--background-active);
  }

  &:before {
    content: "";
    position: absolute;
    left: 0;
    top: 0;
    right: 0;
    bottom: 0;
    border-radius: inherit;
    transition:
      background 0.2s linear,
      transform 0.2s,
      box-shadow 0.2s linear;
    transform: scale(var(--scale, 1)) translateZ(0);
    background: var(--background);
    box-shadow:
      inset 0 0 0 var(--border-width, 1px) var(--border),
      0 4px 8px var(--shadow),
      0 8px 20px var(--shadow);
  }

  .hand {
    width: 11px;
    height: 11px;
    border-radius: 2px 0 0 0;
    background: var(--icon);
    position: relative;
    margin: 10px 8px 0 0;
    transform-origin: -5px -1px;
    transition:
      transform 0.25s,
      background 0.2s linear;
    transform: rotate(calc(var(--hand-rotate, 0) * 1deg)) translateZ(0);
    &:before,
    &:after {
      content: "";
      background: var(--icon);
      position: absolute;
      transition:
        background 0.2s linear,
        box-shadow 0.2s linear;
    }

    &:before {
      left: -5px;
      bottom: 0;
      height: 12px;
      width: 4px;
      border-radius: 1px 1px 0 1px;
    }

    &:after {
      right: -3px;
      top: 0;
      width: 4px;
      height: 4px;
      border-radius: 0 2px 2px 0;
      background: var(--icon);
      box-shadow:
        -0.5px 4px 0 var(--icon),
        -1px 8px 0 var(--icon),
        -1.5px 12px 0 var(--icon);
      transform: scaleY(0.6825);
      transform-origin: 0 0;
    }

    .thumb {
      background: var(--icon);
      width: 10px;
      height: 4px;
      border-radius: 2px;
      transform-origin: 2px 2px;
      position: absolute;
      left: 0;
      top: 0;
      transition:
        transform 0.25s,
        background 0.2s linear;
      transform: scale(0.85) translateY(-0.5px)
        rotate(var(--hand-thumb-1, -45deg)) translateZ(0);
      &:before {
        content: "";
        height: 4px;
        width: 7px;
        border-radius: 2px;
        transform-origin: 2px 2px;
        background: var(--icon);
        position: absolute;
        left: 7px;
        top: 0;
        transition:
          transform 0.25s,
          background 0.2s linear;
        transform: rotate(var(--hand-thumb-2, -45deg)) translateZ(0);
      }
    }
  }

  .hand,
  span {
    display: inline-block;
    vertical-align: top;
    span {
      opacity: var(--span-d-o, 0);
      transition:
        transform 0.25s,
        opacity 0.2s linear;
      transform: translateX(var(--span-d-x, 4px)) translateZ(0);
    }
  }

  & > span {
    margin-left: 8px;
    transition: transform 0.25s;
    transform: translateX(var(--span-x, 4px)) translateZ(0);
  }
}
</style>
