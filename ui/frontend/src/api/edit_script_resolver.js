import { ESRESOLVER_BASE_URL } from "./config";
import axios from "axios";

/**
 * @typedef {Object} ESResolverPayload
 * @property {string} base - The base code.
 * @property {string} ours - The code from the left version.
 * @property {string} theirs - The code from the right version.
 */

/**
 * @typedef {ResolveResult} ResolveResult
 * @property {boolean} resolvable - Whether the conflict is resolvable through this method.
 * @property {string} resolution - The resolved code.
 */

/**
 * @brief get the result from edit script resolver
 *
 * @param {ESResolverPayload} payload conflict data(two versions and the base)
 * @returns {Promise<ResolveResult>} the combine result
 */
export const getEditScriptResolution = async payload => {
  const endpoint = `${ESRESOLVER_BASE_URL}/es_predict`;
  const { data: res } = await axios.post(endpoint, payload, {
    headers: {
      "Content-Type": "application/json",
    },
  });
  if (res) {
    if (res.isSuccessful) return { resolvable: true, resolution: res.data };
    return { resolvable: false, resolution: "" };
  }
  throw new Error(res.msg);
};
