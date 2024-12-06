class Resize {
  constructor() {
    // drag direction
    this._dir = "";
    // prop
    this._prop = "";
    // height or width of container
    this._containerSize = 0;
    this._dragItemList = [];
    // last pos of mouse
    this._last = 0;
    // cache
    this._minSizeCache = {};
    this._maxSizeCache = {};

    this.init = this.init.bind(this);
    this.onDrag = this.onDrag.bind(this);
    this.onDragStart = this.onDragStart.bind(this);
  }

  init({ dir, dragItemList, containerSize }) {
    this._dir = dir;
    this._dragItemList = dragItemList;
    this._containerSize = containerSize;
    this._prop = this._dir === "v" ? "height" : "width";
  }

  getMinSize(index) {
    if (this._minSizeCache[index] !== undefined) {
      return this._minSizeCache[index];
    }
    return (this._minSizeCache[index] =
      ((this._dragItemList[index].min ? this._dragItemList[index].min : 0) /
        this._containerSize) *
      100);
  }

  getMaxSize(index) {
    if (this._maxSizeCache[index] !== undefined) {
      return this._maxSizeCache[index];
    }
    let minSum = this._dragItemList.reduce((sum, cur, i) => {
      return (sum += index === i ? 0 : cur.min || 0);
    }, 0);
    return (this._maxSizeCache[index] =
      ((this._dragItemList[index].max
        ? this._dragItemList[index].max
        : this._containerSize - minSum) /
        this._containerSize) *
      100);
  }

  findFirstNarrowableItem(dir, index) {
    if (dir === "leftUp") {
      let narrowItemIndex = index - 1;
      while (narrowItemIndex >= 0) {
        let _minSize = this.getMinSize(narrowItemIndex);
        if (this._dragItemList[narrowItemIndex][this._prop] > _minSize) {
          break;
        }
        narrowItemIndex--;
      }
      return narrowItemIndex;
    } else if (dir === "rightDown") {
      let narrowItemIndex = index;
      while (narrowItemIndex <= this._dragItemList.length - 1) {
        let _minSize = this.getMinSize(narrowItemIndex);
        if (this._dragItemList[narrowItemIndex][this._prop] > _minSize) {
          break;
        }
        narrowItemIndex++;
      }
      return narrowItemIndex;
    }
  }

  isDraggable(dir, index) {
    if (index === 0 && dir === "rightDown") {
      return false;
    }

    let narrowItemIndex = this.findFirstNarrowableItem(dir, index);
    if (
      narrowItemIndex < 0 ||
      narrowItemIndex > this._dragItemList.length - 1
    ) {
      return false;
    }
    return true;
  }

  onDragStart(e) {
    this._last = this._dir === "v" ? e.clientY : e.clientX;
  }

  onDrag(index, ox, oy, e) {
    let client = this._dir === "v" ? e.clientY : e.clientX;
    // 本次移动的距离
    let dx = client - this._last;
    // 换算成百分比
    let rx = (dx / this._containerSize) * 100;
    // 更新上一次的鼠标位置
    this._last = client;
    if (dx < 0) {
      // 向左/上拖动
      if (!this.isDraggable("leftUp", index)) {
        return;
      }
      // 拖动中的编辑器增加宽度
      if (this._dragItemList[index][this._prop] - rx < this.getMaxSize(index)) {
        this._dragItemList[index][this._prop] -= rx;
      } else {
        this._dragItemList[index][this._prop] = this.getMaxSize(index);
      }
      // 找到左边第一个还有空间的编辑器索引
      let narrowItemIndex = this.findFirstNarrowableItem("leftUp", index);
      let _minSize = this.getMinSize(narrowItemIndex);
      // 左边的编辑器要同比减少宽度
      if (narrowItemIndex >= 0) {
        // 加上本次偏移还大于最小宽度
        if (this._dragItemList[narrowItemIndex][this._prop] + rx > _minSize) {
          this._dragItemList[narrowItemIndex][this._prop] += rx;
        } else {
          // 否则固定为最小宽度
          this._dragItemList[narrowItemIndex][this._prop] = _minSize;
        }
      }
    } else if (dx > 0) {
      // 向右/下拖动
      if (!this.isDraggable("rightDown", index)) {
        return;
      }
      // 找到拖动中的编辑器及其右边的编辑器中的第一个还有空间的编辑器索引
      let narrowItemIndex = this.findFirstNarrowableItem("rightDown", index);
      let _minSize = this.getMinSize(narrowItemIndex);
      if (narrowItemIndex <= this._dragItemList.length - 1) {
        let ax = 0;
        // 减去本次偏移还大于最小宽度
        if (this._dragItemList[narrowItemIndex][this._prop] - rx > _minSize) {
          ax = rx;
        } else {
          // 否则本次能移动的距离为到达最小宽度的距离
          ax = this._dragItemList[narrowItemIndex][this._prop] - _minSize;
        }
        // 更新拖动中的编辑器的宽度
        this._dragItemList[narrowItemIndex][this._prop] -= ax;
        // 左边第一个编辑器要同比增加宽度
        if (index > 0) {
          if (
            this._dragItemList[index - 1][this._prop] + ax <
            this.getMaxSize(index - 1)
          ) {
            this._dragItemList[index - 1][this._prop] += ax;
          } else {
            this._dragItemList[index - 1][this._prop] = this.getMaxSize(
              index - 1,
            );
          }
        }
      }
    }
  }
}

export default Resize;
