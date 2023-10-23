from pymongo import MongoClient

from config import mongo_config

class EvaMongoClient(MongoClient):
    def __init__(self):
        super().__init__(mongo_config.MONGO_CONN_STR)

    def get_database(self) -> MongoClient:
        return self[mongo_config.EVA_DB]
