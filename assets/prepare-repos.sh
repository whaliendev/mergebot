#!/bin/bash

# Define the mapping of projects to their corresponding git URLs and compile_commands.json URL
declare -A repos=(
    [flameshot]="https://github.com/flameshot-org/flameshot.git"
    [duckdb]="https://github.com/duckdb/duckdb.git"
    [rocksdb]="https://github.com/facebook/rocksdb.git"
    [grpc]="https://github.com/grpc/grpc.git"
    [art]="https://android.googlesource.com/platform/art"
    [frameworks_av]="https://android.googlesource.com/platform/frameworks/av"
    [frameworks_native]="https://android.googlesource.com/platform/frameworks/native"
    [system_core]="https://android.googlesource.com/platform/system/core"
    [redis]="https://github.com/redis/redis.git"
    [tmux]="https://github.com/tmux/tmux.git"
)

declare -A compile_json_urls=(
    [flameshot]="https://raw.githubusercontent.com/whaliendev/mergebot-experiment-compdb/refs/heads/main/flameshot.json"
    [duckdb]="https://raw.githubusercontent.com/whaliendev/mergebot-experiment-compdb/refs/heads/main/duckdb.json"
    [rocksdb]="https://raw.githubusercontent.com/whaliendev/mergebot-experiment-compdb/refs/heads/main/rocksdb.json"
    [grpc]="https://raw.githubusercontent.com/whaliendev/mergebot-experiment-compdb/refs/heads/main/grpc.json"
    [art]="https://raw.githubusercontent.com/whaliendev/mergebot-experiment-compdb/refs/heads/main/art.json"
    [frameworks_av]="https://raw.githubusercontent.com/whaliendev/mergebot-experiment-compdb/refs/heads/main/frameworks_av.json"
    [frameworks_native]="https://raw.githubusercontent.com/whaliendev/mergebot-experiment-compdb/refs/heads/main/frameworks_native.json"
    [system_core]="https://raw.githubusercontent.com/whaliendev/mergebot-experiment-compdb/refs/heads/main/system_core.json"
    [redis]="https://raw.githubusercontent.com/whaliendev/mergebot-experiment-compdb/refs/heads/main/redis.json"
    [tmux]="https://raw.githubusercontent.com/whaliendev/mergebot-experiment-compdb/refs/heads/main/tmux.json"
)

# Function to check if git is installed
check_git() {
    if ! command -v git &> /dev/null; then
        echo "git is not installed. Please install git first."
        exit 1
    fi
}

# Function to get the REPOS_DIR from user input
get_repos_dir() {
    echo "Please input the REPOS_DIR (leave blank for default ~/example-repos):"
    read REPOS_DIR
    if [ -z "$REPOS_DIR" ]; then
        REPOS_DIR="$HOME/example-repos"
    fi
    if [ ! -d "$REPOS_DIR" ]; then
        echo "Directory $REPOS_DIR does not exist. Creating it..."
        mkdir -p "$REPOS_DIR"
    fi
}

# Function to clone repo and fetch compile_commands.json
clone_repo_and_fetch_compile_commands() {
    local repo_name=$1
    local repo_url=${repos[$repo_name]}
    local compile_json_url=${compile_json_urls[$repo_name]}
    local repo_dir="$REPOS_DIR/$repo_name"

    if [ -d "$repo_dir" ]; then
        echo "Repository $repo_name already exists. Skipping clone..."
    else
        echo "Cloning repository $repo_name..."
        git clone "$repo_url" "$repo_dir" || { echo "Failed to clone $repo_name"; exit 1; }
    fi

    # Fetch compile_commands.json if it doesn't exist
    if [ ! -f "$repo_dir/compile_commands.json" ]; then
        echo "Fetching compile_commands.json for $repo_name..."
        curl -L "$compile_json_url" -o "$repo_dir/compile_commands.json" || { echo "Failed to download compile_commands.json for $repo_name"; exit 1; }
    else
        echo "compile_commands.json for $repo_name already exists. Skipping download..."
    fi

    # Modify compile_commands.json to replace /mnt/repos/<project_name> with REPOS_DIR/<project_name>
    local compile_json_file="$repo_dir/compile_commands.json"
    if [ -f "$compile_json_file" ]; then
        echo "Modifying compile_commands.json for $repo_name..."
        sed -i "s|/mnt/repos/$repo_name|$repo_dir|g" "$compile_json_file" || { echo "Failed to modify compile_commands.json"; exit 1; }
    else
        echo "compile_commands.json not found after download for $repo_name, skipping modification."
    fi
}

# Function to show help message
show_help() {
    echo "Usage: ./prepare-repos.sh <option> [project_names...]"
    echo ""
    echo "Options:"
    echo "  all                 Clone all repositories and fetch compile_commands.json"
    echo "  <project_name>      Clone a specific repository and fetch compile_commands.json."
    echo "                      Only repositories evaluated in the paper are supported by this script."
    echo "  --help              Show this help message"
    echo ""
    echo "Note:"
    echo "  - Replace <project_name> with the name of one or more predefined repositories (e.g., 'rocksdb')."
    echo "  - You can specify a list of repositories separated by space to clone multiple repositories."
    echo "  - The script will automatically download the compile_commands.json for the specified repositories."
    echo "  - You can use 'all' to clone all repositories at once."
    echo ""
    echo "Example usages:"
    echo "  ./prepare-repos.sh all                # Clone all repositories and fetch compile_commands.json"
    echo "  ./prepare-repos.sh rocksdb            # Clone the 'rocksdb' repository and fetch its compile_commands.json"
    echo "  ./prepare-repos.sh rocksdb duckdb     # Clone 'rocksdb' and 'duckdb' repositories"
    echo "  ./prepare-repos.sh --help             # Display this help message"
}

# Main script
# Check if at least one argument is passed
if [ $# -eq 0 ]; then
    echo "Error: No arguments provided. Use --help for usage instructions."
    exit 1
fi

# Handle script options
case $1 in
    all)
        check_git
        get_repos_dir
        # Clone all repositories
        for repo_name in "${!repos[@]}"; do
            clone_repo_and_fetch_compile_commands "$repo_name"
        done
        echo "All repositories cloned successfully."
        ;;
    --help)
        show_help
        ;;
    *)
        check_git
        get_repos_dir
        # Clone the repositories listed in the arguments
        for repo_name in "$@"; do
            if [[ -z "${repos[$repo_name]}" ]]; then
                echo "Error: Invalid project name '$repo_name'. "
                echo "Current supported repos: ${!repos[@]}"
                echo "Use --help for usage instructions."
                exit 1
            else
                clone_repo_and_fetch_compile_commands "$repo_name"
            fi
        done
        echo "Selected repositories cloned successfully."
        ;;
esac
