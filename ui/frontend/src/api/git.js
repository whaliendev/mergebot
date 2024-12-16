import axios from "axios";

import { BACKEND_BASE_URL as GIT_BASE_URL } from "./config";

/**
 * @typedef {Object} ConflictingMetaResp
 * @property {string} base - base version of the conflicting file
 * @property {ours} ours - our version of the conflicting file
 * @property {theirs} theirs - their version of the conflicting file
 * @property {Array<string>} conflict - array of conflicting file lines
 * @property {string | null} originalConflict - this field is ambiguous, original conflicting file
 * @property {string[]} apply - An array of applied content lines.
 * @property {string|null} fileName - this field is ambiguous, the file name.
 * @property {string} absPath - The absolute path of the file.
 * @property {string} tempPath - The temporary path of the file.
 * @property {string} targetPath - The target path (temporary) of the file.
 * @property {string} sourcePath - The source path (temporary) of the file.
 * @property {string} basePath - The base path (temporary) of the file.
 * @property {string} conflictPath - The path of the conflict file.
 * @property {ConflictBlockMeta[]} info - Information about conflicts.
 */

/**
 * @typedef {Object} ConflictBlockMeta
 * @property {number} mark - this field is ambiguous
 * @property {number} start - the start line of conflict mark(1-based)
 * @property {number} middle - the middle line of conflict mark(1-based)
 * @property {number} middle2 - the middle2 line of conflict mark(1-based)
 * @property {number} end - the end line of conflict mark(1-based)
 * @property {Array<string>} ours - our side file content lines
 * @property {Array<string>} theirs - their side file content lines
 * @property {Array<string>} base - base side file content lines
 * @property {Array<string>} resolve - resolved file content lines
 * @property {Array<string>} historyTruth - history truths, every element is a truth string, not an array
 * @property {Array<number>} truthScore - truth scores, every element is a score number corresponding to historyTruth
 */

/**
 * @typedef {Object} WriteToFilePayload
 * @property {string} path absolute path of the file
 * @property {stirng} fileName basename of the file
 * @property {string} content resolved file content
 * @property {string} repo repository path, absolute path
 * @property {string} tempPath temporary path of the file
 */

/**
 * @brief get the specified conflicting file metadata
 *
 * @param {string} filePath absolute file path of specific file
 * @param {string} repo repo path
 * @returns {Promise<ConflictingMetaResp>} the specified conflicting file metadata
 */
export const getSpecifiedConflictingMeta = async (filePath, repo) => {
  const endpoint = `${GIT_BASE_URL}/conflict/specified?filePath=${filePath}&repo=${repo}&fileType=0`;
  const res = await axios.get(endpoint);
  if (res.status === 200 && res.data.code === 200) {
    const metadata = res.data.data;
    metadata.info = res.data.info;
    return metadata;
  } else {
    return null;
  }
};

/**
 * @brief write resolved file content to file
 *
 * @param {WriteToFilePayload} payload write to file payload
 * response data example: { "msg": "成功写入冲突文件", "code": 200}
 * @returns {Promise<boolean>} true if success, false otherwise
 */
export const writeResolvedToFile = async payload => {
  if (payload.content instanceof Array) {
    payload.content = payload.content.join("\n"); // whether or not to add a newline at the end of the file?
  }

  const endpoint = `${GIT_BASE_URL}/write2file`;

  const formData = new FormData();
  formData.append("path", payload.path);
  formData.append("fileName", payload.fileName);
  formData.append("content", payload.content);
  formData.append("repo", payload.repo);
  formData.append("tempPath", payload.tempPath);

  const res = await axios.put(endpoint, formData, {
    headers: {
      "Content-Type": "multipart/form-data",
    },
  });
  if (res && res.status === 200 && res.data.code === 200) {
    return true;
  } else {
    return false;
  }
};

export const checkFileExistence = async payload => {
  const endpoint = `${GIT_BASE_URL}/file/checkExistence?path=${payload.path}`;
  const res = await axios.get(endpoint);
  if (res && res.status === 200 && res.data.code === 200) {
    return res.data.data;
  }
  return false;
};

export default {
  getSpecifiedConflictingMeta,
  writeResolvedToFile,
  checkFileExistence,
};
