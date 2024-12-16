<template>
  <el-container>
    <el-header>
      <div>
        <el-button icon="el-icon-back" @click="$router.back()"></el-button>
      </div>

      <div>
        <span style="margin-right: 12px">
          {{ this.$route.params.filePath }}
        </span>
        <el-button
          type="primary"
          :disabled="originalFileContent === fileContent"
          @click="write2file"
        >
          Save Edit
        </el-button>
      </div>
    </el-header>

    <el-main>
      <MonacoEditor
        :language="language"
        theme="vs-light"
        v-model="fileContent"
        :options="appliedEditorOptions"
        class="editor"
      ></MonacoEditor>
    </el-main>
  </el-container>
</template>

<script>
import MonacoEditor from "@/components/vue-monaco";
import { getSourceLanguage } from "@/utils";

export default {
  components: {
    MonacoEditor,
  },
  data() {
    return {
      filePath: "",
      fileName: "",
      repo: "",
      language: "",
      fileContent: "",
      originalFileContent: "",
      appliedEditorOptions: {},
    };
  },
  methods: {
    async getFileContent() {
      const { filePath } = this.$route.params;
      try {
        const response = await this.$axios.get("/file/content", {
          params: { path: filePath },
        });
        this.fileContent = response.data.data.join("\n");
        this.originalFileContent = this.fileContent;
      } catch (error) {
        this.$message.error(error.message);
      }
    },

    // todo： 考虑抽取到 requests 中
    async write2file() {
      try {
        // Create a new FormData instance
        const formData = new FormData();

        // Append each field to the FormData object
        formData.append("path", this.filePath);
        formData.append("fileName", this.fileName);
        formData.append("content", this.fileContent);
        formData.append("repo", this.repo);
        formData.append("tempPath", "-1"); // Required by the API

        // Configure Axios to send the FormData
        const res = await this.$axios.put("/write2file", formData, {
          headers: {
            "Content-Type": "multipart/form-data",
          },
        });

        // Handle the response based on the status code
        if (res.data.code === 200) {
          this.$message.success("Save modifications successfully");
          this.originalFileContent = this.fileContent;
        } else {
          this.$message.error("Failed to save modifications");
        }
      } catch (error) {
        // Handle errors gracefully
        this.$message.error(`Error: ${error.message}`);
      }
    },
  },
  created() {
    this.filePath = this.$route.params.filePath;
    this.fileName = this.$route.params.fileName;
    this.repo = this.$route.params.repo;
    this.language = getSourceLanguage(
      this.$route.params.filePath.split(".").pop(),
    );
  },
  mounted() {
    this.getFileContent();
  },
};
</script>

<style scoped>
.el-container {
  height: 100vh;
  /* Takes full viewport height */
  display: flex;
  flex-direction: column;
}

.el-header {
  height: 60px;
  /* Fixed height for header */
  background-color: #b3c0d1;
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.el-main {
  overflow: hidden;
  flex-grow: 1;
  /* Takes up the remaining space */
  display: flex;
  /* This ensures that children can be sized according to the height of el-main */
  flex-direction: column;
  /* Keeps children in vertical order */
}

.editor {
  flex: 1;
  /* Takes up all available space in el-main */
  /* As it's inside el-main which has display: flex, it will stretch to its height */
}
</style>
