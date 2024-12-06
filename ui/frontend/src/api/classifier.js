import axios from "axios";

import { CLASSIFIER_BASE_URL, COMBINE_BASE_URL } from "./config";

/**
 * @typedef {Object} MLPayload
 * @property {string} path - The repository path.
 * @property {string} target - The target branch.
 * @property {string} source - The source branch.
 * @property {string} version1 - The target path with forward slashes.
 * @property {string} version2 - The source path with forward slashes.
 * @property {string} conflict - The conflict path with forward slashes.
 * @property {string} base - The base path with forward slashes.
 * @property {string} filetype - The file type or language.
 */

/**
 * @typedef {Object} MLBlockResolution
 * @property {number} confidence - The confidence level.
 * @property {number} index - The index. (0-based)
 * @property {string} label - The label.
 */

/**
 * @typedef {Object} ConflictData
 * @property {string} version1 - The code from the left version.
 * @property {string} version2 - The code from the right version.
 * @property {string} base - The base code.
 */

/**
 * @brief get the classifier prediction results
 *
 * @param {MLPayload} payload classifier prediction payload
 * @returns {Promise<MLBlockResolution[]>} classifier prediction results
 */
export const getClassifierPredictionResults = async payload => {
  const endpoint = `${CLASSIFIER_BASE_URL}/predict`;
  const res = await axios.post(endpoint, payload, {
    headers: {
      "Content-Type": "application/json",
    },
  });
  if (res && res.status === 200) {
    if (res.data.code !== 200) {
      throw new Error(res.data.msg);
    }
    return res.data.data;
  } else {
    return null;
  }
};

/**
 * @brief get the combine result
 *
 * @param {ConflictData} payload combine resolution payload
 * @returns {Promise<string>} the combine result
 */
export const getCombineResult = async payload => {
  const endpoint = `${COMBINE_BASE_URL}/combine`;
  const { data: res } = await axios.post(endpoint, payload, {
    headers: {
      "Content-Type": "application/json",
    },
  });
  if (res && res.isSuccessful) return res.data;
  throw new Error(res.msg);
};

export default {
  getClassifierPredictionResults,
  getCombineResult,
};
