import axios from "axios";
import qs from "qs";

import { BACKEND_BASE_URL } from "./config.js";

const axios_instance = axios.create({
  baseURL: BACKEND_BASE_URL,
  timeout: 10000,
  headers: {
    "Content-Type": "application/x-www-form-urlencoded",
  },
});

/**
 * @param {string} filePath - path of the added file
 * @param {string} fileType - "file" | "direction" // backend typo for "directory"
 */
export const addFileReq = async (filePath, fileType) => {
  const endpoint = `/files/add`;
  // 后端接口要求使用 application/x-www-form-urlencoded
  const res = await axios_instance.put(
    endpoint,
    qs.stringify({
      filePath,
      fileType,
    }),
  );
  if (res.status !== 200) {
    throw new Error(`Failed to add, status code: ${res.status}`);
  }
  const { code, msg } = res.data;
  if (code !== 200) throw new Error(msg);
};

/**
 * @param {string} filePath - path of the deleted file
 * @param {string} repoPath - repoPath
 */
export const deleteFileReq = async (filePath, repoPath) => {
  const endpoint = `/files/delete`;
  // 后端接口要求使用 application/x-www-form-urlencoded
  const res = await axios_instance.put(
    endpoint,
    qs.stringify({
      filePath,
      repoPath,
    }),
  );
  if (res.status !== 200) {
    throw new Error(`Failed to delete, status code: ${res.status}`);
  }
  const { code, msg } = res.data;
  if (code !== 200) throw new Error(msg);
};

/**
 * @param {string} filePath - path of the deleted file
 * @param {string} newFileName - newFileName
 * @param {string} repoPath - repoPath
 */
export const renameFileReq = async (filePath, newFileName, repoPath) => {
  const endpoint = `/files/rename`;
  // 后端接口要求使用 application/x-www-form-urlencoded
  const res = await axios_instance.put(
    endpoint,
    qs.stringify({
      filePath,
      newFileName,
      repoPath,
    }),
  );
  if (res.status !== 200) {
    throw new Error(`Failed to rename, status code: ${res.status}`);
  }
  const { code, msg } = res.data;
  if (code !== 200) throw new Error(msg);
};

export const uploadFileReq = async (fileList, dirPath) => {
  const endpoint = `/files/upload`;

  const param = new FormData();
  fileList.forEach((val, index) => {
    param.append("file", val.raw);
  });
  param.append("dirPath", dirPath);
  const res = await axios.post(endpoint, param);
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
