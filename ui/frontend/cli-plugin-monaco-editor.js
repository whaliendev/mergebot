const MonacoWebpackPlugin = require("monaco-editor-webpack-plugin");

module.exports = function (api, vueConfig) {
  api.chainWebpack(config => {
    config
      .plugin("monaco-editor")
      .use(MonacoWebpackPlugin, [
        vueConfig.pluginOptions && vueConfig.pluginOptions.monaco,
      ]);
  });
};
