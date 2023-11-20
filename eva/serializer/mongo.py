import logging
import pymongo
from config import mongo_config
from pymongo.errors import ConnectionFailure

logger = logging.getLogger()


def get_mongo_client():
    try:
        mongo_client = pymongo.MongoClient(mongo_config.MONGO_CONN_STR)
    except ConnectionFailure as e:
        logger.error(f"failed to connect to mongodb: {e}")
        raise e
    return mongo_client
