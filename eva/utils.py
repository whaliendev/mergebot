import logging
import time
import functools
from typing import Any, Callable

logger = logging.getLogger(__package__)

def timing(func: Callable[..., Any]) -> Callable[..., Any]:
    @functools.wraps(func)
    def wrapper(*args: Any, **kwargs: Any) -> Any:
        start_time = time.time()
        result = func(*args, **kwargs)
        end_time = time.time()
        elapsed_time_ms = (end_time - start_time) * 1000 # to milliseconds
        logger.info(f"{func.__name__} took {elapsed_time_ms:.4f} ms to run.")
        return result
    return wrapper

def test_timing_decorator() -> None:
    @timing
    def test_func() -> None:
        time.sleep(1)
    test_func()
