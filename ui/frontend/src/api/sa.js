import axios from "axios";

import { SA_BASE_URL } from "./config";

/**
 * @typedef {Object} SAResolutionPayload
 * @property {string} path - The repository path.
 * @property {MergeScenarioPair} ms - The merge scenario.
 * @property {string} file - The absolute file path.
 */

/**
 * @typedef {Object} MergeScenarioPair
 * @property {ours} ours - rev name of our branch
 * @property {theirs} theirs - rev name of their branch
 */

/**
 * @typedef {Object} SABlockResolution
 * @property {string[]} code - An array of strings representing code.
 * @property {string} label - The label.
 * @property {number} index - The index.(0-based)
 * @property {number} confidence - The confidence level.
 * @property {string} desc - The description.
 */

/**
 * @typedef {Object} SAPatchResolution
 * @property {number} start - The starting position.
 * @property {number} offset - The offset.
 * @property {string[]} content - An array of strings representing content.
 */

/**
 * @typedef {Object} SAResolutionResp
 * @property {boolean} pending - Indicates if the SA algorithm is pending.
 * @property {string} projectPath - The project path.
 * @property {string} file - The relative source path.
 * @property {Array<SABlockResolution>} resolutions - An array of resolution objects.
 * @property {Array<SAPatchResolution>} patches - An array of patch objects.
 */

/**
 * @brief get the SA resolution result
 *
 * @param {SAResolutionPayload} payload SA resolution algorithm payload
 * @returns {Promise<SAResolutionResp>} the SA resolution result
 */
export const getSAResolutionResult = async payload => {
  const endpoint = `${SA_BASE_URL}/resolve`;
  const res = await axios.post(endpoint, payload, {
    headers: {
      "Content-Type": "application/json",
    },
  });

  if (res && res.status === 200 && res.data.code === "00000") {
    return res.data.data;
  } else {
    return null;
  }
};

export default {
  getSAResolutionResult,
};
