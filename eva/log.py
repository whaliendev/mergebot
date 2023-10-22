import logging


class ColorfulFormatter(logging.Formatter):
    black = "\033[30m"
    red = "\033[31m"
    green = "\033[32m"
    yellow = "\033[33m"
    blue = "\033[34m"
    magenta = "\033[35m"
    cyan = "\033[36m"
    white = "\033[37m"
    reset = "\033[0m"

    format_template = "%(asctime)s {}[%(levelname)s]{} (%(filename)s:%(lineno)d): %(message)s"

    FORMATS = {
        logging.DEBUG: cyan,
        logging.INFO: green,
        logging.WARNING: yellow,
        logging.ERROR: red,
        logging.CRITICAL: red
    }

    def format(self, record):
        record.levelname = record.levelname.lower()
        level_color = self.FORMATS.get(record.levelno)
        log_fmt = self.format_template.format(level_color, self.reset)
        formatter = logging.Formatter(log_fmt, datefmt='[%Y-%m-%d %H:%M:%S]')
        return formatter.format(record)


# 命令行中的日志级别为INFO，文件中的日志级别为DEBUG
logger = logging.getLogger(__package__)
logger.setLevel(logging.DEBUG)
formatter = logging.Formatter(
    '[%(asctime)s] [%(process)d] [%(levelname)s] (%(filename)s:%(lineno)d): %(message)s')

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
