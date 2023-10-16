import logging


class ColorfulFormatter(logging.Formatter):
    grey = "\x1b[38;20m"
    yellow = "\x1b[33;20m"
    red = "\x1b[31;20m"
    bold_red = "\x1b[31;1m"
    reset = "\x1b[0m"
    format = "%(asctime)s - %(name)s - %(levelname)s - %(message)s (%(filename)s:%(lineno)d)"

    FORMATS = {
        logging.DEBUG: grey + format + reset,
        logging.INFO: grey + format + reset,
        logging.WARNING: yellow + format + reset,
        logging.ERROR: red + format + reset,
        logging.CRITICAL: bold_red + format + reset
    }

    def format(self, record):
        log_fmt = self.FORMATS.get(record.levelno)
        formatter = logging.Formatter(log_fmt)
        return formatter.format(record)


# 配置默认的logger,该logger会同时输出到命令行和文件，命令行中为彩色输出，文件中为无色输出
# 命令行中的日志级别为INFO，文件中的日志级别为DEBUG
logger = logging.getLogger(__package__)
logger.setLevel(logging.DEBUG)
formatter = logging.Formatter(
    '%(asctime)s - %(name)s - %(levelname)s - %(message)s')
# 输出到命令行，命令行需要彩色输出
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
ch.setFormatter(ColorfulFormatter())
logger.addHandler(ch)

# 输出到文件
fh = logging.FileHandler('eva.log')
fh.setLevel(logging.DEBUG)
fh.setFormatter(formatter)
logger.addHandler(fh)
