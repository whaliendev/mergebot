import monaco from "monaco-editor";
import cryptoJs from "crypto-js";

export const getSourceLanguage = fileExt => {
  let language = "unknown";
  const cppExts = new Set([
    "cc",
    "cpp",
    "h",
    "hpp",
    "c++",
    "c",
    "cp",
    "C",
    "cxx",
  ]);
  const javaExts = new Set(["java"]);
  const ktExts = new Set(["kt"]);
  if (cppExts.has(fileExt)) {
    language = "cpp";
  } else if (javaExts.has(fileExt)) {
    language = "java";
  } else if (ktExts.has(fileExt)) {
    language = "kotlin";
  }
  return language;
};

export const findConflictBlocks = content => {
  const conflictBlocks = [];

  let ourMarkerLineNo = -1;
  let baseMarkerLineNo = -1;
  let theirMarkerLineNo = -1;
  let endMarkerLineNo = -1;
  for (let i = 0; i < content.length; i++) {
    const line = content[i];
    if (line.startsWith("<<<<<<<")) {
      ourMarkerLineNo = i + 1;
    } else if (line.startsWith("|||||||") && ourMarkerLineNo > 0) {
      baseMarkerLineNo = i + 1;
    } else if (line.startsWith("=======") && ourMarkerLineNo > 0) {
      theirMarkerLineNo = i + 1;
    } else if (line.startsWith(">>>>>>>") && theirMarkerLineNo > 0) {
      endMarkerLineNo = i + 1;

      conflictBlocks.push({
        ourMarkerLineNo,
        baseMarkerLineNo,
        theirMarkerLineNo,
        endMarkerLineNo,
      });

      ourMarkerLineNo = -1;
      baseMarkerLineNo = -1;
      theirMarkerLineNo = -1;
      endMarkerLineNo = -1;
    }
  }
  return conflictBlocks;
};

export const getConflictBlockDecorations = (
  conflictBlocks,
  ourClassName = "conflicts-ours",
  baseClassName = "conflicts-base",
  theirsClassName = "conflicts-theirs",
  blockClassName = "conflicts-block",
) => {
  const decorations = [];
  conflictBlocks.forEach(block => {
    if (
      block.ourMarkerLineNo > 0 &&
      block.baseMarkerLineNo > 0 &&
      block.theirMarkerLineNo > 0 &&
      block.endMarkerLineNo > 0
    ) {
      // 3-way conflicts
      decorations.push({
        range: new monaco.Range(
          block.ourMarkerLineNo,
          1,
          block.baseMarkerLineNo - 1,
          1,
        ),
        options: {
          // className: ourClassName, // after number decoration
          isWholeLine: true,
          // linesDecorationsClassName: ourClassName, // before linenumber, cldr
          // marginClassName: ourClassName, // most front, cmdr
          glyphMarginClassName: ourClassName, // cgmr
          minimap: {
            color: "#ADD8E6",
            position: monaco.editor.MinimapPosition.Inline,
          },
          overviewRuler: {
            color: "#ADD8E6",
            position: monaco.editor.OverviewRulerLane.Center,
          },
        },
      });

      decorations.push({
        range: new monaco.Range(
          block.baseMarkerLineNo,
          1,
          block.theirMarkerLineNo - 1,
          1,
        ),
        options: {
          // className: baseClassName,
          isWholeLine: true,
          glyphMarginClassName: baseClassName,
          // linesDecorationsClassName: baseClassName,
          // marginClassName: baseClassName,
          minimap: {
            color: "#F5F5F5",
            position: monaco.editor.MinimapPosition.Inline,
          },
          overviewRuler: {
            color: "#F5F5F5",
            position: monaco.editor.OverviewRulerLane.Center,
          },
        },
      });

      decorations.push({
        range: new monaco.Range(
          block.theirMarkerLineNo,
          1,
          block.endMarkerLineNo,
          1,
        ),
        options: {
          // className: theirsClassName,
          isWholeLine: true,
          glyphMarginClassName: theirsClassName,
          // linesDecorationsClassName: theirsClassName,
          // marginClassName: theirsClassName,
          minimap: {
            color: "#F06292",
            position: monaco.editor.MinimapPosition.Inline,
          },
          overviewRuler: {
            color: "#F06292",
            position: monaco.editor.OverviewRulerLane.Center,
          },
        },
      });
    } else if (
      block.ourMarkerLineNo > 0 &&
      block.theirMarkerLineNo > 0 &&
      block.endMarkerLineNo > 0
    ) {
      // traditional 2-way conflicts
      decorations.push({
        range: new monaco.Range(
          block.ourMarkerLineNo,
          1,
          block.theirMarkerLineNo,
          1,
        ),
        options: {
          // className: ourClassName, // after number decoration
          isWholeLine: true,
          // linesDecorationsClassName: ourClassName, // before linenumber, cldr
          // marginClassName: ourClassName, // most front, cmdr
          glyphMarginClassName: ourClassName, // cgmr
          minimap: {
            color: "#ADD8E6",
            position: monaco.editor.MinimapPosition.Inline,
          },
          overviewRuler: {
            color: "#ADD8E6",
            position: monaco.editor.OverviewRulerLane.Center,
          },
        },
      });

      decorations.push({
        range: new monaco.Range(
          block.theirMarkerLineNo,
          1,
          block.endMarkerLineNo,
          1,
        ),
        options: {
          // className: theirsClassName,
          isWholeLine: true,
          glyphMarginClassName: theirsClassName,
          // linesDecorationsClassName: theirsClassName,
          // marginClassName: theirsClassName,
          minimap: {
            color: "#F06292",
            position: monaco.editor.MinimapPosition.Inline,
          },
          overviewRuler: {
            color: "#F06292",
            position: monaco.editor.OverviewRulerLane.Center,
          },
        },
      });
    }

    if (block.ourMarkerLineNo > 0 && block.endMarkerLineNo > 0) {
      decorations.push({
        range: new monaco.Range(
          block.ourMarkerLineNo,
          1,
          block.endMarkerLineNo,
          80,
        ),
        options: {
          isWholeLine: true,
          className: blockClassName,
        },
      });
    }
  });

  return decorations;
};

export const getPatchDecorations = (patches, patchClass = "patch-glyph") => {
  const decorations = [];
  patches.forEach(patch => {
    decorations.push({
      range: new monaco.Range(
        patch.start,
        1,
        patch.start + patch.offset - 1,
        80,
      ),
      options: {
        isWholeLine: true,
        glyphMarginClassName: patchClass,
        overviewRuler: {
          color: "#FFA07A",
          position: monaco.editor.OverviewRulerLane.Center,
        },
      },
    });
  });
  return decorations;
};

/**
 * get [start, end] of an array
 * @param {Array<string>} arr
 * @param {Number} start
 * @param {Number} end
 */
export const getArraySlice = (arr, start, end) => {
  if (start < 0 || end >= arr.length || start > end) {
    return []; // 或者返回一个错误信息
  }

  // 获取[start, end]范围内的元素
  const result = arr.slice(start, end + 1);

  return result;
};

/**
 * calc SHA256 of [start, end] of lines
 * @param {Array<string>} lines
 * @param {Number} start 0-based
 * @param {Number} end 0-based
 */
export const getBlockHash = (lines, start, end) => {
  const block = getArraySlice(lines, start, end);
  // console.log(block);
  return cryptoJs.SHA256(block.join("\n")).toString(cryptoJs.enc.Hex);
};

export const findSpecificFingerprint = (lines, finger) => {
  for (let i = 0; i < lines.length; i++) {
    if (lines[i].startsWith("<<<<<<<")) {
      // debugger;
      const block = [];
      let start = i,
        end = i;
      block.push(lines[i]);
      i++;
      while (lines.length > i && !lines[i].startsWith(">>>>>>>")) {
        block.push(lines[i]);
        i++;
      }
      if (lines.length > i) {
        end = i;
        block.push(lines[i]);
        // console.log(block);
        // debugger;
        if (finger === getBlockHash(lines, start, end)) {
          return { start, end };
        }
      }
      continue;
    }
  }

  return {
    start: -1,
    end: -1,
  };
};
