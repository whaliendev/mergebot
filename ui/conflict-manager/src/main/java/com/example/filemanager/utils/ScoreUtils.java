package com.example.filemanager.utils;

public class ScoreUtils {
    //检查文本相似度
    public double checkSimilarity(String x, String y) {
        double maxLength = Double.max(x.length(), y.length());
        if (maxLength > 0) {
            // 如果需要，可以选择忽略大小写
            return (maxLength - getLevenshteinDistance(x, y)) / maxLength;
        }
        return 1.0;
    }

    private static int min(int a, int b, int c) {
        if (b < a) {
            a = b;
        }
        if (c < a) {
            a = c;
        }
        return a;
    }


    public static int getLevenshteinDistance(String s, String t) {
        if (s != null && t != null) {
            String shorter_str, longer_str;
            if (s.length() < t.length()) {
                shorter_str = s;
                longer_str = t;
            } else {
                shorter_str = t;
                longer_str = s;
            }
            int mi = shorter_str.length(), ma = longer_str.length();

            if (mi == 0) {
                return ma;
            }
            int[] d = new int[mi + 1];

            int j;
            for (j = 0; j <= mi; d[j] = j++) {
            }
            for(int i = 1; i <= ma; ++i) {
                int pre = i - 1;    // 左上
                char l_i = longer_str.charAt(i - 1);
                d[0] = i;

                for(j = 1; j <= mi; ++j) {
                    char s_j = shorter_str.charAt(j - 1);
                    byte cost;
                    if (l_i == s_j) {
                        cost = 0;
                    } else {
                        cost = 1;
                    }
                    int tmp = d[j];
                    d[j] = min(d[j] + 1, d[j - 1] + 1, pre + cost);
                    pre = tmp;
                }
            }
            return d[mi];
        } else {
            throw new IllegalArgumentException("Strings must not be null");
        }
    }
}
