#!/bin/env python
from gitservice import DumpTask, dump_tree_objects_par

def main():
    dump_tasks = [
        DumpTask(dest='/tmp/framworks_av/target', hash='b9b352687e03c02958348bd25b9c4b056ebe6d63', repo_path='/home/whalien/Desktop/frameworks_av'),
        DumpTask(dest='/tmp/framworks_av/source', hash='fa24d44e438a88657246edf2bd7c4142c04b0f7e', repo_path='/home/whalien/Desktop/frameworks_av'),
        DumpTask(dest='/tmp/framworks_av/base', hash='559114ac1c7f6254a7199d37e53b98bc7341fb6a', repo_path='/home/whalien/Desktop/frameworks_av')
    ]

    dump_tree_objects_par(dump_tasks)

if __name__ == '__main__':
    main()
