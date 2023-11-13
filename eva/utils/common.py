import logging
import time
import functools
from typing import Any, Callable
from pygit2.ffi import ffi
import os

logger = logging.getLogger(__package__)


def timing(func: Callable[..., Any]) -> Callable[..., Any]:
    @functools.wraps(func)
    def wrapper(*args: Any, **kwargs: Any) -> Any:
        start_time = time.time()
        result = func(*args, **kwargs)
        end_time = time.time()
        elapsed_time_ms = (end_time - start_time) * 1000  # to milliseconds
        logger.info(f"{func.__name__} took {elapsed_time_ms:.4f} ms to run.")
        return result

    return wrapper


def to_bytes(s, encoding="utf-8", errors="strict"):
    if s == ffi.NULL or s is None:
        return ffi.NULL

    if hasattr(s, "__fspath__"):
        s = os.fspath(s)

    if isinstance(s, bytes):
        return s

    return s.encode(encoding, errors)


def to_str(s):
    if hasattr(s, "__fspath__"):
        s = os.fspath(s)

    if type(s) is str:
        return s

    if type(s) is bytes:
        return s.decode()

    raise TypeError(f'unexpected type "{repr(s)}"')


def test_timing_decorator() -> None:
    @timing
    def test_func() -> None:
        time.sleep(1)

    test_func()
