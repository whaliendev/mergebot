import rminer from "@/api/refactoring";
import Vue from "vue";

const state = () => ({
  refactorings: [
    {
      type: "Extract Method",
      description: "Extracts the selected code into a new method",
      leftSideLocations: [
        {
          filePath: "src/components/MyComponent.vue",
          startLine: 10,
          endLine: 20,
          startColumn: 0,
          endColumn: 0,
          codeElementType: "method",
          description: "The code to be extracted",
          codeElement: "extractedCode",
        },
      ],
      rightSideLocations: [
        {
          filePath: "src/components/MyComponent.vue",
          startLine: 10,
          endLine: 20,
          startColumn: 0,
          endColumn: 0,
          codeElementType: "method",
          description: "The code to be extracted",
          codeElement: "extractedCode",
        },
      ],
    },
  ],

  showRefactroingIdx: -1,
});

const getters = {
  refactoringDrawerList: state => {
    const drawerListData = [];
    let idx = 0;
    for (const refactoring of state.refactorings) {
      drawerListData.push({
        idx: idx++,
        type: refactoring.type,
        description: refactoring.description,
      });
    }
    return drawerListData;
  },

  refactoringToShow: state => {
    if (state.showRefactroingIdx < 0) return null;
    if (state.showRefactroingIdx >= state.refactorings.length) return null;

    return state.refactorings[state.showRefactroingIdx];
  },
};

const actions = {
  async getRefactorings({ commit }, payload) {
    const refactorings = await rminer.getJavaFileRefactorings(
      payload.filePath,
      payload.targetContent,
      payload.sourceContent,
    );
    // console.log(refactorings);
    if (refactorings) {
      commit("setRefactorings", refactorings);
    } else {
      commit("clearRefactorings");
    }
  },
};

const mutations = {
  setRefactorings(state, refactorings) {
    // console.log("Setting refactorings", refactorings);
    state.refactorings = refactorings;
  },

  clearRefactorings(state) {
    state.refactorings = [];
  },

  setShowRefactoringIdx(state, idx) {
    if (idx < -1 || idx >= state.refactorings.length) {
      Vue.$prototype.$message({
        type: "error",
        message: "不存在序号为" + idx + "的重构项",
      });
    }
    state.showRefactroingIdx = idx;
  },
};

export default {
  namespaced: true,
  state,
  getters,
  actions,
  mutations,
};
