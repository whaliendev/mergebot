from pymongo import MongoClient

from eva.config import MONGO_CONFIG


def get_database():
    client = MongoClient(MONGO_CONFIG["MONGO_CONN_STR"])
    return client[MONGO_CONFIG["EVA_DB"]]
