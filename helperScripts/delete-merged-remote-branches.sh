#!/bin/bash

# Script to delete merged remote branches
# Usage: ./delete-merged-remote-branches.sh [--dry-run] [--remote origin]

set -e

# Default values
DRY_RUN=false
REMOTE="origin"
MAIN_BRANCH="main"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        --remote)
            REMOTE="$2"
            shift 2
            ;;
        --main-branch)
            MAIN_BRANCH="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --dry-run           Show what would be deleted without actually deleting"
            echo "  --remote NAME       Specify remote name (default: origin)"
            echo "  --main-branch NAME  Specify main branch name (default: main)"
            echo "  -h, --help          Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0 --dry-run                    # Preview what would be deleted"
            echo "  $0 --remote upstream            # Use 'upstream' remote"
            echo "  $0 --main-branch master         # Use 'master' as main branch"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Merged Remote Branch Cleanup Script                  ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════╝${NC}"
echo ""

# Verify git repository
if ! git rev-parse --git-dir > /dev/null 2>&1; then
    echo -e "${RED}Error: Not in a git repository${NC}"
    exit 1
fi

# Fetch latest remote data
echo -e "${BLUE}Fetching latest data from remote '${REMOTE}'...${NC}"
git fetch "$REMOTE" --prune

# Verify remote exists
if ! git remote | grep -q "^${REMOTE}$"; then
    echo -e "${RED}Error: Remote '${REMOTE}' does not exist${NC}"
    echo "Available remotes:"
    git remote -v
    exit 1
fi

# Verify main branch exists
if ! git show-ref --verify --quiet "refs/remotes/${REMOTE}/${MAIN_BRANCH}"; then
    echo -e "${YELLOW}Warning: Branch '${REMOTE}/${MAIN_BRANCH}' not found${NC}"
    echo "Trying 'master'..."
    MAIN_BRANCH="master"
    if ! git show-ref --verify --quiet "refs/remotes/${REMOTE}/${MAIN_BRANCH}"; then
        echo -e "${RED}Error: Could not find main branch. Use --main-branch to specify${NC}"
        exit 1
    fi
fi

echo -e "${GREEN}✓ Using remote: ${REMOTE}${NC}"
echo -e "${GREEN}✓ Using main branch: ${MAIN_BRANCH}${NC}"
echo ""

# Protected branches (never delete these)
PROTECTED_BRANCHES=(
    "$MAIN_BRANCH"
    "master"
    "develop"
    "development"
    "staging"
    "production"
)

# Get list of remote branches
echo -e "${BLUE}Finding merged branches...${NC}"
echo ""

# Get merged branches
MERGED_BRANCHES=$(git branch -r --merged "${REMOTE}/${MAIN_BRANCH}" | \
    grep "${REMOTE}/" | \
    grep -v " -> " | \
    sed "s/^[[:space:]]*//" | \
    sed "s|^${REMOTE}/||")

# Filter out protected branches
BRANCHES_TO_DELETE=()
while IFS= read -r branch; do
    # Skip empty lines
    [[ -z "$branch" ]] && continue
    
    # Check if branch is protected
    IS_PROTECTED=false
    for protected in "${PROTECTED_BRANCHES[@]}"; do
        if [[ "$branch" == "$protected" ]]; then
            IS_PROTECTED=true
            break
        fi
    done
    
    if [[ "$IS_PROTECTED" == false ]]; then
        BRANCHES_TO_DELETE+=("$branch")
    fi
done <<< "$MERGED_BRANCHES"

# Display results
if [[ ${#BRANCHES_TO_DELETE[@]} -eq 0 ]]; then
    echo -e "${GREEN}✓ No merged branches to delete${NC}"
    echo ""
    echo "All remote branches are either:"
    echo "  - Not merged yet"
    echo "  - Protected branches"
    exit 0
fi

echo -e "${YELLOW}Found ${#BRANCHES_TO_DELETE[@]} merged branch(es) to delete:${NC}"
echo ""

for branch in "${BRANCHES_TO_DELETE[@]}"; do
    echo -e "  ${RED}✗${NC} ${REMOTE}/${branch}"
done

echo ""

# Confirm or execute
if [[ "$DRY_RUN" == true ]]; then
    echo -e "${BLUE}[DRY RUN] No branches were deleted${NC}"
    echo ""
    echo "To actually delete these branches, run:"
    echo "  $0 --remote ${REMOTE}"
    exit 0
fi

# Ask for confirmation (unless --yes flag is added later)
echo -e "${YELLOW}Are you sure you want to delete these branches from remote '${REMOTE}'?${NC}"
read -p "Type 'yes' to confirm: " -r
echo ""

if [[ ! $REPLY =~ ^[Yy][Ee][Ss]$ ]]; then
    echo -e "${BLUE}Cancelled. No branches were deleted.${NC}"
    exit 0
fi

# Delete branches
echo -e "${BLUE}Deleting merged branches...${NC}"
echo ""

DELETED_COUNT=0
FAILED_COUNT=0

for branch in "${BRANCHES_TO_DELETE[@]}"; do
    echo -n "Deleting ${REMOTE}/${branch}... "
    
    if git push "$REMOTE" --delete "$branch" > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Deleted${NC}"
        ((DELETED_COUNT++))
    else
        echo -e "${RED}✗ Failed${NC}"
        ((FAILED_COUNT++))
    fi
done

echo ""
echo -e "${BLUE}╔════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Summary                                               ║${NC}"
echo -e "${BLUE}╠════════════════════════════════════════════════════════╣${NC}"
echo -e "${BLUE}║${NC}  ${GREEN}Deleted:${NC} ${DELETED_COUNT} branch(es)                              ${BLUE}║${NC}"
if [[ $FAILED_COUNT -gt 0 ]]; then
    echo -e "${BLUE}║${NC}  ${RED}Failed:${NC}  ${FAILED_COUNT} branch(es)                              ${BLUE}║${NC}"
fi
echo -e "${BLUE}╚════════════════════════════════════════════════════════╝${NC}"
echo ""

if [[ $DELETED_COUNT -gt 0 ]]; then
    echo -e "${GREEN}✓ Successfully cleaned up merged remote branches!${NC}"
fi

