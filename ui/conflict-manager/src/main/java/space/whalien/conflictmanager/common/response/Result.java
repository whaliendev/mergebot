package space.whalien.conflictmanager.common.response;

import space.whalien.conflictmanager.common.error.ErrorCode;

public class Result {
    static final String DEFAULT_SUCCESS_CODE = "200";
    static final String DEFAULT_SUCCESS_MSG = "成功";

    private final String code;
    private final String msg;
    private final Object data;

    private Result(String code, String msg, Object data) {
        this.code = code;
        this.msg = msg;
        this.data = data;
    }

    public static <T> Result success() {
        return new Result(DEFAULT_SUCCESS_CODE, DEFAULT_SUCCESS_MSG, null);
    }

    public static <T> Result success(T data) {
        return new Result(DEFAULT_SUCCESS_CODE, DEFAULT_SUCCESS_MSG, data);
    }

    public static Result error(ErrorCode errorCode) {
        return new Result(errorCode.getCode(), errorCode.getMessage(), null);
    }

    public static Result error(String code, String msg) {
        return new Result(code, msg, null);
    }

    public String getCode() {
        return code;
    }

    public String getMsg() {
        return msg;
    }

    public Object getData() {
        return data;
    }
}

