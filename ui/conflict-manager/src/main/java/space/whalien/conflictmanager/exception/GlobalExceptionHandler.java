package space.whalien.conflictmanager.exception;

import org.apache.tomcat.util.http.fileupload.impl.FileSizeLimitExceededException;
import org.apache.tomcat.util.http.fileupload.impl.SizeLimitExceededException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.web.bind.annotation.ControllerAdvice;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.ResponseBody;
import org.springframework.web.multipart.MaxUploadSizeExceededException;
import org.springframework.web.multipart.MultipartException;

@ControllerAdvice
public class GlobalExceptionHandler {
    private final static Logger logger = LoggerFactory.getLogger(GlobalExceptionHandler.class);

    @ExceptionHandler(value = MultipartException.class)
    @ResponseBody
    public String multipartExceptionHandler(MaxUploadSizeExceededException ex) {
        String msg;
        if (ex.getCause().getCause() instanceof FileSizeLimitExceededException) {
            logger.error(ex.getMessage());
            msg = "上传文件过大[单文件大小不得超过5M]";
        } else if (ex.getCause().getCause() instanceof SizeLimitExceededException) {
            logger.error(ex.getMessage());
            msg = "上传文件过大[总上传文件大小不得超过50]";
        } else {
            msg = "上传文件失败";
        }

        return msg;
    }
}
