package space.whalien.conflictmanager.utils;

import info.debatty.java.stringsimilarity.Cosine;
import org.springframework.data.util.Pair;
import space.whalien.conflictmanager.exception.IllegalBranchNameException;
import space.whalien.conflictmanager.pojo.vo.BlockResolutionChoiceVO;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class AuditUtils {
    private final static int GIT_MAX_HASH_SIZE = 40;
    public static String getFileMD5(String projectPath, String targetBranch, String sourceBranch, String fileName) throws IOException {
        String targetHash= GitUtils.getFullHash(targetBranch, projectPath);
        String sourceHash = GitUtils.getFullHash(sourceBranch, projectPath);

        if (targetHash.length() != GIT_MAX_HASH_SIZE || sourceHash.length() != GIT_MAX_HASH_SIZE) {
            throw new IllegalBranchNameException(projectPath, targetBranch);
        }

        // check projectPath + fileName exists
        Path filePath = Paths.get(projectPath, fileName);
        if (!Files.exists(filePath)) {
            throw new IOException(String.format("文件 %s 不存在", filePath));
        }

        // get file md5
        return getMD5Identifier(projectPath, targetHash, sourceHash, fileName);
    }

    public static Pair<Float, String> calcSimilarity(BlockResolutionChoiceVO vo) {
        // TODO(hwa): remove hard encoded choice code
        float similarity = -1;
        String choice = "";
        String choiceCode = vo.getChoiceCode();
        String saCode = vo.getSaCode();
        if (null != saCode && !saCode.isEmpty()) {
            similarity = stringSimilarity(choiceCode, saCode);
            choice = "sa";
        }

        String mlCode = vo.getMlCode();
        if (null != mlCode && !mlCode.isEmpty()) {
            float mlSimilarity = stringSimilarity(choiceCode, mlCode);
            if (mlSimilarity > similarity) {
                similarity = mlSimilarity;
                choice = "ml";
            }
        }

        String dlCode = vo.getDlCode();
        if (null != dlCode && !dlCode.isEmpty()) {
            float dlSimilarity = stringSimilarity(choiceCode, dlCode);
            if (dlSimilarity > similarity) {
                similarity = dlSimilarity;
                choice = "dl";
            }
        }
        return Pair.of(similarity, choice);
    }

    public static float stringSimilarity(String s1, String s2) {
        Cosine cosine = new Cosine();
        return (float) cosine.similarity(s1, s2);
    }

    private static String getMD5Identifier(String projectPath, String targetHash, String sourceHash, String fileName) {
        try {
            MessageDigest md = MessageDigest.getInstance("MD5");

            String combined = projectPath + targetHash + sourceHash + fileName;
            byte[] bytes = combined.getBytes();

            byte[] digest = md.digest(bytes);

            StringBuilder hexString = new StringBuilder();
            for (byte b : digest) {
                String hex = Integer.toHexString(0xff & b);
                if (hex.length() == 1) hexString.append('0');
                hexString.append(hex);
            }

            return hexString.toString();
        } catch (NoSuchAlgorithmException e) {
            throw new RuntimeException("Could not find MD5 algorithm", e);
        }
    }
}
