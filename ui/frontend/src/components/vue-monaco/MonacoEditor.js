import assign from "nano-assign";

export default {
  name: "MonacoEditor",

  props: {
    original: String,
    value: {
      type: String,
      required: true,
    },
    theme: {
      type: String,
      default: "vs",
    },
    language: String,
    options: Object,
    amdRequire: {
      type: Function,
    },
    diffEditor: {
      type: Boolean,
      default: false,
    },
  },

  model: {
    event: "change",
  },

  watch: {
    options: {
      deep: true,
      handler(options) {
        if (this.editor) {
          const editor = this.getModifiedEditor();
          editor.updateOptions(options);
        }
      },
    },

    value(newValue) {
      if (this.editor) {
        const editor = this.getModifiedEditor();
        if (newValue !== editor.getValue()) {
          editor.setValue(newValue);
        }
      }
    },

    language(newVal) {
      if (this.editor) {
        const editor = this.getModifiedEditor();
        this.monaco.editor.setModelLanguage(editor.getModel(), newVal);
      }
    },

    theme(newVal) {
      if (this.editor) {
        this.monaco.editor.setTheme(newVal);
      }
    },
  },

  mounted() {
    if (this.amdRequire) {
      this.amdRequire(["vs/editor/editor.main"], () => {
        this.monaco = window.monaco;
        this.initMonaco(window.monaco);
      });
    } else {
      // ESM format so it can't be resolved by commonjs `require` in eslint
      const monaco = require("monaco-editor");
      this.monaco = monaco;
      this.initMonaco(monaco);
    }
  },

  beforeDestroy() {
    this.editor && this.editor.dispose();
  },

  methods: {
    initMonaco(monaco) {
      const options = assign(
        {
          value: this.value,
          theme: this.theme,
          language: this.language,
        },
        this.options,
      );

      if (this.diffEditor) {
        this.editor = monaco.editor.createDiffEditor(this.$el, options);
        const originalModel = monaco.editor.createModel(
          this.original,
          this.language,
        );
        const modifiedModel = monaco.editor.createModel(
          this.value,
          this.language,
        );
        this.editor.setModel({
          original: originalModel,
          modified: modifiedModel,
        });
      } else {
        this.editor = monaco.editor.create(this.$el, options);
      }

      // @event `change`
      const editor = this.getModifiedEditor();
      editor.onDidChangeModelContent(event => {
        const value = editor.getValue();
        if (this.value !== value) {
          this.$emit("change", value, event);
        }
      });

      this.$emit("editorDidMount", this.editor);
    },

    /** @deprecated */
    getMonaco() {
      return this.editor;
    },

    getEditor() {
      return this.editor;
    },

    getModifiedEditor() {
      return this.diffEditor ? this.editor.getModifiedEditor() : this.editor;
    },

    focus() {
      this.editor.focus();
    },

    setLanguage(lang) {
      if (this.editor) {
        const editor = this.getModifiedEditor();
        this.monaco.editor.setModelLanguage(editor.getModel(), lang);
      }
    },
  },

  render(h) {
    return h("div");
  },
};
