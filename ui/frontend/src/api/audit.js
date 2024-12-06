import axios from "axios";
import { BACKEND_BASE_URL } from "./config.js";

/**
 * @brief 点赞文件
 *
 * @param {string} projectPath 项目路径
 * @param {string} targetBranch 目标分支
 * @param {string} sourceBranch 源分支
 * @param {string} fileName 文件名
 * @returns {Promise<Result>} 点赞结果
 */
export const thumbUpFile = async params => {
  const endpoint = `${BACKEND_BASE_URL}/audit/like`;
  try {
    const response = await axios.post(endpoint, null, { params });
    return response.data;
  } catch (error) {
    throw error.response.data;
  }
};

/**
 * @brief 取消点赞文件
 *
 * @param {string} projectPath 项目路径
 * @param {string} targetBranch 目标分支
 * @param {string} sourceBranch 源分支
 * @param {string} fileName 文件名
 * @returns {Promise<Result>} 取消点赞结果
 */
export const thumbDownFile = async params => {
  const endpoint = `${BACKEND_BASE_URL}/audit/dislike`;
  try {
    const response = await axios.post(endpoint, null, { params });
    return response.data;
  } catch (error) {
    throw error.response.data;
  }
};

/**
 * @brief 保存块解决方案选择
 *
 * @param {Object} request 块解决方案选择请求对象，包含以下字段：
 *   - {string} projectPath 项目路径
 *   - {string} targetBranch 目标分支
 *   - {string} sourceBranch 源分支
 *   - {string} fileName 文件名
 *   - {number} blockIdx 块索引
 *   - {Object} blockResolutionChoice 块解决方案选择对象，包含以下字段：
 *     - {string} choice 选择
 *     - {string} choiceCode 选择代码
 *     - {string} saCode SA代码
 *     - {string} mlCode 机器学习代码
 *     - {string} dlCode 深度学习代码
 * @returns {Promise<Result>} 保存结果
 */
export const saveBlockResolutionChoice = async request => {
  const endpoint = `${BACKEND_BASE_URL}/audit/choice/save`;

  try {
    const response = await axios.post(endpoint, request);
    return response.data;
  } catch (error) {
    throw error.response.data;
  }
};

/**
 * @brief 保存文件跟踪数据
 *
 * @param {string} projectPath 项目路径
 * @param {string} targetBranch 目标分支
 * @param {string} sourceBranch 源分支
 * @param {string} fileName 文件名
 * @param {number} blockCnt 块数量
 * @returns {Promise<Result>} 保存结果
 */
export const saveFileTrackingData = async params => {
  const endpoint = `${BACKEND_BASE_URL}/audit/tracking/save`;
  try {
    const response = await axios.post(endpoint, null, { params });
    return response.data;
  } catch (error) {
    throw error.response.data;
  }
};

export default {
  thumbUpFile,
  thumbDownFile,
  saveBlockResolutionChoice,
  saveFileTrackingData,
};
