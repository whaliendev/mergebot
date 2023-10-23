from dataclasses import dataclass
from pathlib import Path
import os

@dataclass
class MongoConfig:
    MONGO_CONN_STR: str
    EVA_DB: str
    MB_COLLECTION: str
    GIT_COLLECTION: str
    META_COLLECTION: str

@dataclass
class EvaConfig:
    BASE_DIR: str

config = EvaConfig(
    BASE_DIR = os.path.join(Path.home, '.mergebot', 'eva'),
)

mongo_config = MongoConfig(
    MONGO_CONN_STR = 'mongodb://localhost:27017',
    EVA_DB = 'mergebot',
    MB_COLLECTION = 'mergebot',
    GIT_COLLECTION = 'git',
    META_COLLECTION = 'meta',
)
