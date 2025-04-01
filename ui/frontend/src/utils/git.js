import { diffLines } from "diff";
import { diff_match_patch, DIFF_EQUAL, DIFF_INSERT } from "diff-match-patch";
import { findConflictBlocks } from "./merge";

/**
 * @typedef {Object} DiffHunk
 * @property {number} start
 * @property {number} offset
 * @property {string} oldContent
 * @property {string} newContent
 */

// Many of the methods above return change objects. These objects consist of the following fields:

// value: Text content
// added: True if the value was inserted into the new string
// removed: True if the value was removed from the old string
// Note that some cases may omit a particular flag field. Comparison on the flag fields should always be done in a truthy or falsy manner.

/**
 *
 * @param {string} original
 * @param {string} target
 * @return {Array<DiffHunk>}
 */
/**
 * Calculate the diff between original and target content and return an array of DiffHunk objects.
 * @param {string} original
 * @param {string} target
 * @return {Array<DiffHunk>}
 */
export const customGitDiff = (original, target, startLine = -1) => {
  const differences = diffLines(original, target);

  let hunks = [];
  let currentHunk = null;
  let lineNo = 1;

  for (const diff of differences) {
    if (diff.added || diff.removed) {
      if (!currentHunk) {
        currentHunk = {
          start: lineNo,
          offset: 0,
          oldContent: "",
          newContent: "",
        };
      }

      if (diff.added) {
        currentHunk.newContent += diff.value;
      } else {
        currentHunk.oldContent += diff.value;
        currentHunk.offset = diff.count;
        lineNo += diff.count;
      }
    } else {
      if (currentHunk) {
        hunks.push(currentHunk);
        currentHunk = null;
      }
      lineNo += diff.count;
    }
  }

  hunks = hunks.filter(hunk => {
    if (hunk.start + hunk.offset - 1 < startLine) return false;
    return (
      hunk.oldContent.replace(/\s/g, "") !== hunk.newContent.replace(/\s/g, "")
    );
  });

  return hunks;
};

/**
 * @typedef {Array} DiffChunk
 * @property {number} 0 - 0 for no difference, -1 for deletion, 1 for insertion
 * @property {string} 1 - the text that is deleted or inserted
 */

/**
 * gmpDiffLines calculates the difference between two text inputs line by line.
 * It uses the Google's Diff Match Patch library to perform the diff operation.
 *
 * @param {string} original - The original text.
 * @param {string} target - The target text to compare with the original.
 * @returns {Array<DiffChunk>} diffs - An array of differences. Each difference is a 2-element array where:
 *                          - the first element is a number: 0 for no difference, -1 for deletion, 1 for insertion
 *                          - the second element is the text that is deleted or inserted
 */
const gmpDiffLines = (original, target) => {
  const dmp = new diff_match_patch();
  const a = dmp.diff_linesToChars_(original, target);
  const lineText1 = a.chars1;
  const lineText2 = a.chars2;
  const lineArray = a.lineArray;
  const diffs = dmp.diff_main(lineText1, lineText2, false);
  dmp.diff_charsToLines_(diffs, lineArray);
  // dmp.diff_cleanupSemantic(diffs);
  return diffs;
};

export const customGitDiff2 = (original, target, startLine = -1) => {
  const differences = gmpDiffLines(original, target);

  let hunks = [];
  let currentHunk = null;
  let lineNo = 1;

  for (const diff of differences) {
    if (diff[0] !== DIFF_EQUAL) {
      if (!currentHunk) {
        currentHunk = {
          start: lineNo,
          offset: 0,
          oldContent: "",
          newContent: "",
        };
      }

      if (diff[0] === DIFF_INSERT) {
        currentHunk.newContent += diff[1];
      } else {
        currentHunk.oldContent += diff[1];
        currentHunk.offset = diff[1].split("\n").length - 1;
        lineNo += currentHunk.offset;
      }
    } else {
      if (currentHunk) {
        hunks.push(currentHunk);
        currentHunk = null;
      }
      lineNo += diff[1].split("\n").length - 1;
    }
  }

  hunks = hunks.filter(hunk => {
    if (hunk.start + hunk.offset - 1 < startLine) return false;
    return (
      hunk.oldContent.replace(/\s/g, "") !== hunk.newContent.replace(/\s/g, "")
    );
  });

  return hunks;
};

export const constructSemanticPatch = (original, hunks) => {
  const conflictBlocks = findConflictBlocks(original);
  let mergedHunks = [];
  let consideredHunks = [];
  conflictBlocks.forEach(block => {
    const intersectingHunks = hunks.filter(hunk => {
      const hunkStart = hunk.start,
        hunkEnd = hunk.start + hunk.offset - 1;
      const blockStart = block.ourMarkerLineNo,
        blockEnd = block.endMarkerLineNo;
      return hunkEnd >= blockStart && hunkStart <= blockEnd;
    });

    if (intersectingHunks.length) {
      consideredHunks.push(...intersectingHunks.map(hunk => hunk.start));
      intersectingHunks.sort((a, b) => a.start - b.start);

      const start = Math.min(intersectingHunks[0].start, block.ourMarkerLineNo);
      const offset =
        Math.max(
          intersectingHunks[intersectingHunks.length - 1].start +
            intersectingHunks[intersectingHunks.length - 1].offset -
            1,
          block.endMarkerLineNo, // fix: replace theirMarkerLineNo with endMarkerLineNo
        ) -
        start +
        1;

      let newContent = "";
      let lastEnd = start;
      intersectingHunks.forEach(hunk => {
        if (hunk.start > lastEnd) {
          if (newContent.length > 0) newContent += "\n";
          newContent += original.slice(lastEnd - 1, hunk.start - 1).join("\n");
        }
        if (newContent.length > 0 && !hunk.newContent.startsWith("\n"))
          newContent += "\n";
        newContent += hunk.newContent;
        lastEnd = hunk.start + hunk.offset;
      });
      if (lastEnd < start + offset) {
        if (newContent.length > 0) newContent += "\n";
        newContent += original
          .slice(lastEnd - 1, start + offset - 1)
          .join("\n");
      }

      mergedHunks.push({
        start,
        offset,
        newContent,
      });
    }
  });

  const leftHunks = hunks.filter(hunk => !consideredHunks.includes(hunk.start));

  mergedHunks = mergedHunks.concat(leftHunks).sort((a, b) => a.start - b.start);

  return mergedHunks;
};
