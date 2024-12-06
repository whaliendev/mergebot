import Vue from "vue";
import Vuex from "vuex";
import diffEditorStore from "./modules/diff-editor";
import refactoringStore from "./modules/refactoring";

Vue.use(Vuex);

const debug = process.env.NODE_ENV !== "production";

export default new Vuex.Store({
  modules: {
    diff: diffEditorStore,
    refactoring: refactoringStore,
  },
  strict: debug,
});
