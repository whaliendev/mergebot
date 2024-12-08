package com.example.filemanager.common.response;

import com.example.filemanager.common.error.ErrorCode;

public class Result<T> {
    static final String DEFAULT_SUCCESS_CODE = "200";
    static final String DEFAULT_SUCCESS_MSG = "成功";

    private final String code;
    private final String msg;
    private final T data;

    private Result(String code, String msg, T data) {
        this.code = code;
        this.msg = msg;
        this.data = data;
    }

    public static <T> Result<T> success() {
        return new Result<>(DEFAULT_SUCCESS_CODE, DEFAULT_SUCCESS_MSG, null);
    }

    public static <T> Result<T> success(T data) {
        return new Result<>(DEFAULT_SUCCESS_CODE, DEFAULT_SUCCESS_MSG, data);
    }

    public static Result<?> error(ErrorCode errorCode) {
        return new Result<>(errorCode.getCode(), errorCode.getMessage(), null);
    }

    public static Result<?> error(String code, String msg) {
        return new Result<>(code, msg, null);
    }

    public String getCode() {
        return code;
    }

    public String getMsg() {
        return msg;
    }

    public T getData() {
        return data;
    }
}

