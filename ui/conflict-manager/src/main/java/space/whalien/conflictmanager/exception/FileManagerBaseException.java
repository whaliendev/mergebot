package space.whalien.conflictmanager.exception;

/**
 * FileManagerBaseException 用来表示一种可以处理的异常，将拼接的 message 返回到前端
 */
public class FileManagerBaseException extends RuntimeException{
    public FileManagerBaseException(String message) {
        super(message);
    }

    public FileManagerBaseException(String message, Throwable cause) {
        super(message, cause);
    }
}
