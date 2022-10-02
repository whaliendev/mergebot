#!/bin/bash
# 
# run all the sanitization tasks before committing
# this script is simplified from 
# google's [grpc project](https://github.com/grpc/grpc/blob/master/tools/distrib/sanitize.sh)

# fail fast and leave messages for us
set -ex 

cd $(dirname $0)/../..

tools/sanitization/iwyu.sh || true

