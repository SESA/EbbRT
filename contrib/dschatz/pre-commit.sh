#!/bin/bash
#set -x

# To enable this Git hook, copy file to: <Your-Repo>/.git/hooks/pre-commit

git diff --full-index --binary > /tmp/stash.$$
git stash -q --keep-index

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
GIT_ROOT=$DIR/../..
RESULT=0
while read status file
do
    clang-format -style=file "$GIT_ROOT/$file" | \
        diff "$GIT_ROOT/$file" -
    if [ $? -ne 0 ]; then
        echo "$file is not clang-formatted"
        let RESULT=1
        continue
    fi
    $GIT_ROOT/contrib/dschatz/cpplint.py \
        --filter=-runtime/references,-build/include_order,-readability/streams "$GIT_ROOT/$file"
    if [ $? -ne 0 ]; then
        let RESULT=1
        continue
    fi
done < <(git diff --cached --name-status --diff-filter=ACM | \
    grep -P '\.((cc)|(h))$')

if [ -s /tmp/stash.$$ ]; then
    git apply --whitespace=nowarn < /tmp/stash.$$ && git stash drop -q
else
    git stash drop -q
fi

rm /tmp/stash.$$

[ $RESULT -ne 0 ] && exit 1
exit 0
