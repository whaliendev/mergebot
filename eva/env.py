import logging

from pymongo import MongoClient
from config import mongo_config

from log import ColorfulFormatter


def init_env() -> tuple[MongoClient, logging.Logger]:
    """Initialize the environment for the program.

    :return: a tuple of (mongo_client, logger)
    """

    # create logger
    logger = logging.getLogger(__package__)
    logger.setLevel(logging.DEBUG)
    formatter = logging.Formatter(
        "[%(asctime)s] [%(process)d] [%(levelname)s] (%(filename)s:%(lineno)d): %(message)s"
    )

    # output to console
    ch = logging.StreamHandler()
    ch.setLevel(logging.INFO)
    ch.setFormatter(ColorfulFormatter())
    logger.addHandler(ch)

    # output to file
    fh = logging.FileHandler("eva.log")
    fh.setLevel(logging.DEBUG)
    fh.setFormatter(formatter)
    logger.addHandler(fh)

    mongo_client = MongoClient(mongo_config.MONGO_CONN_STR)

    return mongo_client, logger
