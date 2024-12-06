import Vue from "vue";

import "./index.css";

import ElementUI from "element-ui";
import "element-ui/lib/theme-chalk/index.css"; // 引入样式

import App from "./App.vue";
import router from "./router";
import store from "./store";
// import EventEmitter from "eventemitter3";
import axios from "axios";

// 关闭dialog滚动条
ElementUI.Dialog.props.lockScroll.default = false;
Vue.prototype.$axios = axios;
// Vue.prototype.$eventBus = new EventEmitter();
axios.defaults.baseURL = "http://localhost:8080/";

Vue.config.productionTip = false;

Vue.use(ElementUI);

new Vue({
  router,
  store,
  render: h => h(App),
}).$mount("#app");
