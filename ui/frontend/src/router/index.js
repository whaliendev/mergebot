import Vue from "vue";
import VueRouter from "vue-router";
import PureEditor from "@/views/PureEditor.vue";
import FileView from "@/views/FileView.vue";

import BlockResolution from "@/views/BlockResolution.vue";
import DiffResolution from "@/views/DiffResolution.vue";
import DiffResolved from "@/views/DiffResolved.vue";

Vue.use(VueRouter);

export const BlockResolutionView = "BlockResolution";
export const DiffResolutionView = "DiffResolution";
export const DiffResolvedView = "DiffResolved";
export const PureEditorView = "PureEditor";
export const FileTreeView = "FileView";

const routes = [
  {
    path: "/",
    redirect: "/fileview",
  },
  {
    path: "/fileview",
    name: FileTreeView,
    component: FileView,
    props: route => ({
      repoRoute: route.query.repo,
      targetRoute: route.query.target,
      sourceRoute: route.query.source,
      compdbRoute: route.query.compdb,
    }),
  },
  {
    path: "/resolve/block",
    name: BlockResolutionView,
    component: BlockResolution,
    // component: () =>
    //   import(
    //     /* webpackChunkName: "BlockResolution" */ "../views/BlockResolution.vue"
    //   ),
  },
  {
    path: "/resolve/diff",
    name: DiffResolutionView,
    component: DiffResolution,
    // component: () =>
    //   import(
    //     /* webpackChunkName: "DiffResolution" */ "../views/DiffResolution.vue"
    //   ),
  },
  {
    path: "/resolved/diff",
    name: DiffResolvedView,
    component: DiffResolved,
  },
  {
    path: "/editor",
    name: PureEditorView,
    component: PureEditor,
  },
];

const router = new VueRouter({
  mode: "history",
  base: process.env.BASE_URL,
  routes,
});

export default router;
