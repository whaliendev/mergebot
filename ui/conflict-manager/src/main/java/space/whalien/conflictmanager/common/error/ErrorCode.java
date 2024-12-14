package space.whalien.conflictmanager.common.error;

/**
 * 预定义的错误码，C端异常用C开头，S端异常用S开头
 */
public enum ErrorCode {
    PARAM_ERROR("C10000", "参数不合法"),
    UNAUTHORIZED_ACCESS("C10001", "未授权"),
    FORBIDDEN_ACCESS("C10002", "禁止访问"),
    RESOURCE_NOT_FOUND("C10003", "资源不存在"),
    METHOD_NOT_ALLOWED("C10004", "方法不允许"),

    SYSTEM_ERROR("S10000", "file manager java 服务端异常"),
    SYSTEM_BUSY("S10001", "系统繁忙，请稍后再试"),
    SYSTEM_TIMEOUT("S10002", "系统超时"),
    SYSTEM_LIMIT("S10003", "系统限流"),
    SYSTEM_DOWN("S10004", "系统宕机"),
    SYSTEM_RESOURCE_ERROR("S10005", "系统资源异常"),
    SYSTEM_CONFIG_ERROR("S10006", "系统配置异常"),
    SYSTEM_NOT_IMPLEMENTED_YET("S10007", "该接口未实现")
    ;


    private final String code;
    private final String message;

    ErrorCode(String code, String message) {
        this.code = code;
        this.message = message;
    }

    public String getCode() {
        return code;
    }

    public String getMessage() {
        return message;
    }
}


