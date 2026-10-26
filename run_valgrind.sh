#!/usr/bin/env bash
# run_valgrind.sh - memory check (leaks, invalid reads/writes, uninitialised
# values) for the Library Management System tests and the app itself.
#
# Usage:   ./run_valgrind.sh            (run from the folder with the Makefile)
# Logs:    build/valgrind/<name>.log    (one log per program)
# Exit:    0 = everything clean, 1 = valgrind reported a problem

set -u

if ! command -v valgrind >/dev/null 2>&1; then
    echo "valgrind not found. Install it with: sudo apt install valgrind"
    exit 2
fi

BUILD=build
LOGDIR=$BUILD/valgrind
TESTS="test_utils test_storage test_book test_member test_transaction"

# 1. build everything (extra GTEST_INC / GTEST_LIBS can be passed via environment)
make all || { echo "Build failed"; exit 2; }
mkdir -p "$LOGDIR"

VG="valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
--error-exitcode=1"

fail=0
report() {   # report <name> <exit-code>
    if [ "$2" -eq 0 ]; then
        printf '  PASS  %s\n' "$1"
    else
        printf '  FAIL  %s  (see %s/%s.log)\n' "$1" "$LOGDIR" "$1"
        fail=1
    fi
}

echo "Running valgrind..."

# 2. the GoogleTest binaries
for t in $TESTS; do
    $VG --log-file="$LOGDIR/$t.log" "./$BUILD/$t" >/dev/null 2>&1
    report "$t" $?
done

# 3. the real program: start it and exit straight away (menu choice 0).
#    Run inside a scratch folder so books.txt etc. are not created here.
APP="$PWD/$BUILD/library_app"
WORK=$(mktemp -d)
( cd "$WORK" && printf '0\n' | $VG --log-file="$OLDPWD/$LOGDIR/library_app.log" "$APP" >/dev/null 2>&1 )
report library_app $?
rm -rf "$WORK"

echo
if [ $fail -eq 0 ]; then
    echo "All programs are clean: no leaks, no memory errors."
else
    echo "Problems found. Open the logs in $LOGDIR/ for details."
fi
exit $fail
