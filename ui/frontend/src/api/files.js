import axios from "axios";
import { BACKEND_BASE_URL } from "./config.js";

const axios_instance = axios.create({
  baseURL: BACKEND_BASE_URL,
  timeout: 10000,
});

export const addFileReq = async (filePath, fileType) => {
  const endpoint = `/files/add`;
  const formData = new FormData();
  formData.append("filePath", filePath);
  formData.append("fileType", fileType);

  const res = await axios_instance.put(endpoint, formData);
  if (res.status !== 200) {
    throw new Error(`Failed to add, status code: ${res.status}`);
  }
  const { code, msg } = res.data;
  if (code !== 200) throw new Error(msg);
};

export const deleteFileReq = async (filePath, repoPath) => {
  const endpoint = `/files/delete`;
  const formData = new FormData();
  formData.append("filePath", filePath);
  formData.append("repoPath", repoPath);

  const res = await axios_instance.put(endpoint, formData);
  if (res.status !== 200) {
    throw new Error(`Failed to delete, status code: ${res.status}`);
  }
  const { code, msg } = res.data;
  if (code !== 200) throw new Error(msg);
};

export const renameFileReq = async (filePath, newFileName, repoPath) => {
  const endpoint = `/files/rename`;
  const formData = new FormData();
  formData.append("filePath", filePath);
  formData.append("newFileName", newFileName);
  formData.append("repoPath", repoPath);

  const res = await axios_instance.put(endpoint, formData);
  if (res.status !== 200) {
    throw new Error(`Failed to rename, status code: ${res.status}`);
  }
  const { code, msg } = res.data;
  if (code !== 200) throw new Error(msg);
};

export const uploadFileReq = async (fileList, dirPath) => {
  const endpoint = `/files/upload`;

  const formData = new FormData();
  fileList.forEach(val => {
    formData.append("file", val.raw);
  });
  formData.append("dirPath", dirPath);

  const res = await axios_instance.post(endpoint, formData);
  if (res.status !== 200) {
    throw new Error(`Failed to upload, status code: ${res.status}`);
  }
  const { code, msg } = res.data;
  if (code !== 200) throw new Error(msg);
};

export default {
  addFileReq,
  deleteFileReq,
  renameFileReq,
  uploadFileReq,
};
