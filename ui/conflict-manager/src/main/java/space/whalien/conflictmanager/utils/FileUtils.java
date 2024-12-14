package com.example.filemanager.utils;

import difflib.DiffUtils;
import difflib.Patch;
import org.apache.tika.detect.DefaultDetector;
import org.apache.tika.detect.Detector;
import org.apache.tika.io.TikaInputStream;
import org.apache.tika.metadata.Metadata;
import org.apache.tika.mime.MediaType;

import java.io.*;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class FileUtils {
    private static final int BINARY_CHECK_MAX_BYTES = 8192;

    public static boolean isFileExists(String path) {
        java.io.File file = new java.io.File(path);
        return file.exists() && file.isFile();
    }

    //将文件读取为list形式
    public List<String> readFile(File file) throws Exception {
        List<String> codes = new ArrayList<>();
        InputStreamReader isr = new InputStreamReader(new FileInputStream(file));
        BufferedReader reader = new BufferedReader(isr);

        String line;
        while ((line = reader.readLine()) != null) {
            codes.add(line);
        }
        isr.close();
        reader.close();
        return codes;
    }

    public void write(String path, byte[] bytes) throws Exception {

        File file = new File(path);
        if (file.exists()) {
            if (!file.delete()) {
                throw new Exception("file failed to be deleted");
            }
        }

        FileOutputStream fos = new FileOutputStream(file);
        if (bytes != null) {
            fos.write(bytes, 0, bytes.length);
        }
        fos.flush();
        fos.close();
    }

    public int[] alignLines(List<String> conflict, List<String> resolve) {
        int n = conflict.size();
        int m = resolve.size();
        int[] rec = new int[n];
        Arrays.fill(rec, -1);

        Patch<String> patch = DiffUtils.diff(conflict, resolve);
        List<String> diff = DiffUtils.generateUnifiedDiff("", "", conflict, patch, Math.max(n, m));

        for (int i = 0, j = 0, k = 3; k < diff.size(); ++k) {
            char c = diff.get(k).charAt(0);
            if (c == '-')
                i++;
            else if (c == '+')
                j++;
            else {
                rec[i] = j;
                i++;
                j++;
            }
        }
        return rec;
    }

    public List<String> getCodeSnippets(List<String> code, int start, int end){
        if(start >= end)
            return new ArrayList<>();
        return code.subList(start + 1, end);
    }

    public  String getFileType(String filePath) throws IOException {
        InputStream in = new FileInputStream(filePath);
        byte[] bytes = new byte[100]; // 读取文件的前10个字节
        in.read(bytes);
        in.close();

        boolean isText = true;
        for (byte b : bytes) {
            if (b < 0x09 || b > 0x7F) { // 判断字节是否在ASCII码范围内
                isText = false;
                break;
            }
        }

        if (isText) {
            return "text";
        } else {
            return "binary";
        }
    }

    public boolean isBinaryFile(String filePath) {
        if(filePath.endsWith(".xml")){
            return false;
        }
        Detector detector = new DefaultDetector();
        try (TikaInputStream stream = TikaInputStream.get(new File(filePath))){
            Metadata metadata = new Metadata();
            MediaType mediaType = detector.detect(stream, metadata);
            stream.close();
            return !mediaType.getType().equals("text");
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }

    /**
     * Check if a file is binary or text.
     * <p>
     *     This method reads the first {@value #BINARY_CHECK_MAX_BYTES} bytes of the file to determine if it is binary or text.
     *     If the file is larger than {@value #BINARY_CHECK_MAX_BYTES} bytes, only the first {@value #BINARY_CHECK_MAX_BYTES} bytes are read.
     *     If the file is smaller than {@value #BINARY_CHECK_MAX_BYTES} bytes, the entire file is read.
     *     We first check for a BOM (Byte Order Mark) to determine the encoding of the file, as most text files will have a u8 bom.
     *     Next, we check if the file contains any null bytes. If it does, it is binary.
     *     Finally, we check the non-printable characters and printable characters ratio. If the ratio is greater than 1/128, it is binary.
     * </p>
     * @param filePath the path of the file
     * @return true if the file is binary, false if it is text
     * @throws IOException if an I/O error occurs
     * @throws IllegalArgumentException if the file does not exist or is a directory
     */
    public static boolean isBinaryFile2(String filePath) throws IOException, IllegalArgumentException {
        File file = new File(filePath);
        if (!file.exists() || file.isDirectory()) {
            throw new IllegalArgumentException("File does not exist or is a directory: " + filePath);
        }

        byte[] buffer = new byte[BINARY_CHECK_MAX_BYTES];
        try (FileInputStream fis = new FileInputStream(file)) {
            int bytesRead = fis.read(buffer);

            BOMType[] bom = new BOMType[1];
            int bomLength = detectBOM(bom, buffer);

            if (bom[0].ordinal() > BOMType.UTF8.ordinal()) {
                return true; // most source files will be saved in UTF-8
            }

            // here we use startIndex to avoid copying the buffer, use length to skip possible nil bytes at the end
            return isBinaryData(buffer, bomLength, bytesRead);
        }
    }

    private enum BOMType {
        NONE,
        UTF8,
        UTF16_LE,
        UTF16_BE,
        UTF32_LE,
        UTF32_BE
    }

    private static int detectBOM(BOMType[] bom, byte[] buf) {
        bom[0] = BOMType.NONE;
        if (buf.length < 2) {
            return 0;
        }

        int len = buf.length;

        switch (buf[0]) {
            case 0:
                if (len >= 4 && buf[1] == 0 && buf[2] == (byte) 0xFE && buf[3] == (byte) 0xFF) {
                    bom[0] = BOMType.UTF32_BE;
                    return 4;
                }
                break;
            case (byte) 0xEF:
                if (len >= 3 && buf[1] == (byte) 0xBB && buf[2] == (byte) 0xBF) {
                    bom[0] = BOMType.UTF8;
                    return 3;
                }
                break;
            case (byte) 0xFE:
                if (buf[1] == (byte) 0xFF) {
                    bom[0] = BOMType.UTF16_BE;
                    return 2;
                }
                break;
            case (byte) 0xFF:
                if (buf[1] != (byte) 0xFE) {
                    break;
                }
                if (len >= 4 && buf[2] == 0 && buf[3] == 0) {
                    bom[0] = BOMType.UTF32_LE;
                    return 4;
                } else {
                    bom[0] = BOMType.UTF16_LE;
                    return 2;
                }
            default:
                break;
        }

        return 0;
    }

    private static boolean isBinaryData(byte[] data, int startIndex, int length) {
        int printable = 0, nonprintable = 0;

        for (int i = startIndex; i < length; i++) {
            int c = data[i] & 0xFF; // Convert byte to unsigned int

            if ((c > 0x1F && c != 127) || c == '\b' || c == '\033' || c == '\014') {
                printable++;
            } else if (c == '\0') {
                return true;
            } else if (!Character.isWhitespace(c)) {
                nonprintable++;
            }
        }

        // heuristic method taken from libgit2
        // see https://github.com/libgit2/libgit2/blob/e5e233caedaa96d6bdb35a7e42b9028b6dfcf534/src/util/str.c#L1241
        return ((printable >> 7) < nonprintable);
    }

}



