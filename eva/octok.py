#!/bin/env python

import logging
import os
from queue import Queue
from typing import List
import click
import sys
from utils.gitservice import get_repo
from command.miner import mine_repos_conflicts

import log


@click.group()
def cli():
    pass


@cli.command()
def evaluate():
    pass


@cli.command()
@click.option("--limit", default=sys.maxsize, help="Number of Merge Scenarios to mine")
@click.option(
    "--sample-projects-limit",
    default=100,
    help="upper limit of sample projects to mine",
)
@click.option(
    "--repo",
    "-r",
    help="Directory of single git project",
    multiple=True,
    type=click.Path(exists=True, file_okay=False, dir_okay=True),
)
@click.argument(
    "sample", nargs=1, type=click.Path(exists=True, file_okay=False, dir_okay=True)
)
def mine(
    limit: int,
    sample_projects_limit: int,
    repo: List[str],
    sample: str,
):
    logger = logging.getLogger()
    repos = []
    sample_is_repo = True
    try:
        sample_repo = get_repo(sample)
    except ValueError as e:
        sample_is_repo = False
        logger.info(
            f"${sample} is not a valid git repository, we will try to mine it as a directory of git projects"
        )

    if sample_is_repo:
        repos.append(sample)
    else:
        dir_que = Queue()
        dir_que.put(sample)

        while not dir_que.empty() and len(repos) < sample_projects_limit:
            cur_dir = dir_que.get()
            for child in os.listdir(cur_dir):
                child_path = os.path.join(cur_dir, child)
                if os.path.isdir(child_path):
                    path_is_repo = True
                    try:
                        get_repo(child_path)
                    except ValueError as e:
                        dir_que.put(child_path)
                        path_is_repo = False
                    if path_is_repo:
                        repos.append(child_path)

    for r in repo:
        path_is_repo = True
        try:
            get_repo(r)
        except ValueError as e:
            logger.error(f"${r} is not a valid git repository, skipping it")
            path_is_repo = False
        if path_is_repo:
            repos.append(r)

    logger.info(f"mining git projects: {repos}")
    mine_repos_conflicts(repos, limit)


@cli.command()
def pull():
    pass


if __name__ == "__main__":
    cli()
