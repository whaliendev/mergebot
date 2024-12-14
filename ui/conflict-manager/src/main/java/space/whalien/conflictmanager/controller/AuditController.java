package space.whalien.conflictmanager.controller;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.data.util.Pair;
import org.springframework.web.bind.annotation.*;
import space.whalien.conflictmanager.common.response.Result;
import space.whalien.conflictmanager.pojo.ResolutionChoice;
import space.whalien.conflictmanager.pojo.vo.BlockResolutionChoiceRequest;
import space.whalien.conflictmanager.pojo.vo.BlockResolutionChoiceVO;
import space.whalien.conflictmanager.services.AuditService;
import space.whalien.conflictmanager.utils.AuditUtils;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

@CrossOrigin
@RestController
@RequestMapping("/audit")
public class AuditController {
    private static final Logger logger = LoggerFactory.getLogger(AuditController.class);
    private final AuditService auditService;

    // lazy handling here
    private static final Set<String> CHOICES_NEED_CALC_SIMILARITY = new HashSet<>(Arrays.asList("base", "ours", "theirs"));
    private static final Set<String> CHOICES_NEED_NOT_CALC_SIMILARITY = new HashSet<>(Arrays.asList("sa", "ml", "dl"));

    public AuditController(AuditService auditService) {
        this.auditService = auditService;
    }

    @PostMapping("/like")
    Result thumbUpFile(@RequestParam("projectPath") String projectPath, @RequestParam("targetBranch") String targetBranch,
                          @RequestParam("sourceBranch") String sourceBranch, @RequestParam("fileName") String fileName) {
        String fileMD5;
        try {
            fileMD5 = AuditUtils.getFileMD5(projectPath, targetBranch, sourceBranch, auditService.getRelPath(fileName));
        } catch (Exception e) {
            logger.error("Failed to get file MD5", e);
            return Result.error("400", e.getMessage()); // if we fail to get md5, it means argument is invalid
        }

        try {
            Boolean success = auditService.thumbUpFile(fileMD5);

            if (!success) {
                return Result.error("500", "Like failed");
            }
        }catch (Exception e){
            logger.error("Failed to thumb up file", e);
            return Result.error("500", "Like failed");
        }

        return Result.success();
    }

    @PostMapping("/dislike")
    Result thumbDownFile(@RequestParam("projectPath") String projectPath, @RequestParam("targetBranch") String targetBranch,
                            @RequestParam("sourceBranch") String sourceBranch, @RequestParam("fileName") String fileName) {
        String fileMD5;
        try {
            fileMD5 = AuditUtils.getFileMD5(projectPath, targetBranch, sourceBranch, auditService.getRelPath(fileName));
        } catch (Exception e) {
            logger.error("Failed to get file MD5", e);
            return Result.error("400", e.getMessage());
        }

        // save file like status
        try {
            Boolean success = auditService.thumbDownFile(fileMD5);
            if (!success) {
                return Result.error("500", "Unlike failed");
            }
        }catch (Exception e){
            logger.error("Failed to thumb down file", e);
            return Result.error("500", "Unlike failed");
        }

        return Result.success();
    }

    @PostMapping("/choice/save")
    Result saveBlockResolutionChoice(@RequestBody BlockResolutionChoiceRequest request) {
        // filemanager:track:fileMD5, set blockIdx to ResolutionChoice
        String projectPath = request.getProjectPath();
        String targetBranch = request.getTargetBranch();
        String sourceBranch = request.getSourceBranch();
        String fileName = request.getFileName();
        Integer blockIdx = request.getBlockIdx();
        BlockResolutionChoiceVO blockResolutionChoice = request.getBlockResolutionChoice();

        String fileMD5;
        try {
            fileMD5 = AuditUtils.getFileMD5(projectPath, targetBranch, sourceBranch, auditService.getRelPath(fileName));
        } catch (Exception e) {
            logger.error("Failed to get file MD5", e);
            return Result.error("400", e.getMessage());
        }
        // extract enum ?
        String choice = blockResolutionChoice.getChoice();
        if (null == choice || choice.isEmpty()) {
            return Result.error("400", "Tracking point selection cannot be empty.");
        }
        if (!CHOICES_NEED_CALC_SIMILARITY.contains(choice) && !CHOICES_NEED_NOT_CALC_SIMILARITY.contains(choice)) {
            return Result.error("400", "Tracking point selection illegal");
        }

        ResolutionChoice resolutionChoice = null;
        if (CHOICES_NEED_CALC_SIMILARITY.contains(choice)) {
            Pair<Float, String> similarity = AuditUtils.calcSimilarity(blockResolutionChoice);
            resolutionChoice = new ResolutionChoice(null, null, blockIdx, choice, similarity.getFirst(), similarity.getSecond());
        } else {
            resolutionChoice = new ResolutionChoice(null, null, blockIdx, choice, 1f, choice);
        }

        try {
            Boolean success = auditService.saveBlockResolutionChoice(fileMD5, blockIdx, resolutionChoice);
            if (!success) {
                return Result.error("500", "Save the selections of tracking points failed");
            }
        }catch (Exception e){
            logger.error("Failed to save block resolution choice", e);
            return Result.error("500", "Save the selections of tracking points failed");
        }

        return Result.success();
    }

    @PostMapping("/tracking/save")
    Result saveFileTrackingData(@RequestParam("projectPath") String projectPath, @RequestParam("targetBranch") String targetBranch,
                                   @RequestParam("sourceBranch") String sourceBranch, @RequestParam("fileName") String fileName,
                                   @RequestParam("blockCnt") Integer blockCnt) {
        String fileMD5;
        try {
            fileMD5 = AuditUtils.getFileMD5(projectPath, targetBranch, sourceBranch, auditService.getRelPath(fileName));
        } catch (Exception e) {
            return Result.error("400", e.getMessage());
        }

        // save file tracking data
        try {
            Boolean success = auditService.saveFileTrackingData(fileMD5, projectPath, targetBranch,
                    sourceBranch, auditService.getRelPath(fileName), blockCnt);
            if(!success) {
                return Result.error("500", String.format("Fail to save the tracking data of file %s", auditService.getRelPath(fileName)));
            }
        } catch (Exception e) {
            logger.error("Failed to save file tracking data", e);
            return Result.error("500", String.format("Fail to save the tracking data of file %s", auditService.getRelPath(fileName)));
        }

        return Result.success();
    }
}
