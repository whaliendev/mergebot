import os
import subprocess
import logging

logger = logging.getLogger(__name__)

def expand_path(path: str) -> str:
    expanded_path = os.path.expanduser(os.path.expandvars(path))

    if not os.path.isabs(expanded_path):
        expanded_path = os.path.abspath(expanded_path)

    return expanded_path

def remove_cpp_comments(input_path, output_path):
    """use gcc -fpreprocessed -dD -P -E input_path > output_path to remove comments"""
    if not os.path.exists(input_path):
        logger.error(f"input_path does not exist")
        exit(1)

    subprocess.run(
        ["gcc", "-fpreprocessed", "-dD", "-P", "-E", input_path],
        stdout=open(output_path, "w"),
        check=True,
    )
