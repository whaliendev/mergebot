import { REFACTORING_BASE_URL } from "./config";
import axios from "axios";

export const getJavaFileRefactorings = async (
  filePath,
  targetContent,
  sourceContent,
) => {
  const endpoint = `${REFACTORING_BASE_URL}/api/refactor/java/detect/file`;
  const targetBlob = new Blob([targetContent], { type: "text/plain" });
  const sourceBlob = new Blob([sourceContent], { type: "text/plain" });
  const formData = new FormData();
  formData.append("filePath", filePath);
  formData.append("targetFile", targetBlob, filePath);
  formData.append("sourceFile", sourceBlob, filePath);

  const res = await axios.post(endpoint, formData, {
    headers: {
      "Content-Type": "multipart/form-data",
    },
  });

  if (res && res.status === 200 && res.data.code === "00000") {
    return res.data.data;
  }
  throw new Error(res.data.msg);
};

export default {
  getJavaFileRefactorings,
};
