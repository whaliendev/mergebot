import pymongo
from config import mongo_config


def get_mongo_client():
    mongo_client = pymongo.MongoClient(mongo_config.MONGO_CONN_STR)
    return mongo_client
