import logging
from .. import log

def test_colorful_formatter():
    # create a log record with level WARNING
    record = logging.LogRecord(
        name='test_logger',
        level=logging.WARNING,
        pathname='/path/to/file.py',
        lineno=10,
        msg='This is a test message',
        args=None,
        exc_info=None
    )

    # create an instance of ColorfulFormatter
    formatter = log.ColorfulFormatter()

    # format the log record using ColorfulFormatter
    formatted = formatter.format(record)

    # check that the formatted string contains the expected color codes
    assert '\033[33m' in formatted  # yellow color for WARNING level
    assert '\033[0m' in formatted  # reset color code
    assert 'This is a test message' in formatted
