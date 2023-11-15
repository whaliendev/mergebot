from dataclasses import dataclass
from pathlib import Path
import os


@dataclass
class MongoConfig:
    MONGO_CONN_STR: str
    EVA_DB: str
    PROJECT_COLLECTION: str
    MS_COLLECTION: str
    CS_COLLECTION: str


@dataclass
class EvaConfig:
    BASE_DIR: str


config = EvaConfig(
    BASE_DIR=os.path.join(Path.home(), ".mergebot", "eva"),
)

mongo_config = MongoConfig(
    MONGO_CONN_STR="mongodb://localhost:27017",
    EVA_DB="merge",
    PROJECT_COLLECTION="projects",
    MS_COLLECTION="merge_scenarios",
    CS_COLLECTION="conflict_sources",
)
