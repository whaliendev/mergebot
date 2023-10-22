from pymongo import MongoClient

from config import mongo_config


def get_database() -> MongoClient:
    client = MongoClient(mongo_config.MONGO_CONN_STR)
    return client[mongo_config.EVA_DB]
