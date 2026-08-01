#!/bin/bash

# Finds Qt and the cmake/ninja toolchain on a macOS dev box, exports QTDIR and
# prepends them to PATH. No-op when QTDIR is already set, so CI keeps its own.

QT_SEARCH_PATHS=(
    "~/Qt/<version>/macos"
    "/opt/Qt/<version>/macos"
    "/usr/local/Qt/<version>/macos"
    "/Applications/Qt/<version>/macos"
    "brew --prefix qt (or qt@6)"
    "/opt/local/libexec/qt6"
    "qmake6 or qmake on PATH"
)

qt_prefix_from_qmake() {
    local qmake
    qmake=$(command -v "$1" 2>/dev/null) || return 0
    "$qmake" -query QT_INSTALL_PREFIX 2>/dev/null || return 0
}

find_qt() {
    local candidates=() dir prefix root
    # find, not a glob: zsh aborts on an unmatched one, and this file is also
    # useful sourced straight from a macOS shell.
    for root in "$HOME/Qt" /opt/Qt /usr/local/Qt /Applications/Qt; do
        [ -d "$root" ] || continue
        while IFS= read -r dir; do
            candidates+=("$dir")
        done < <(find "$root" -maxdepth 2 -mindepth 2 -type d -name macos 2>/dev/null)
    done
    if command -v brew >/dev/null 2>&1; then
        for dir in qt qt@6; do
            if prefix=$(brew --prefix "$dir" 2>/dev/null); then
                candidates+=("$prefix")
            fi
        done
    fi
    candidates+=("/opt/local/libexec/qt6")
    candidates+=("$(qt_prefix_from_qmake qmake6)" "$(qt_prefix_from_qmake qmake)")

    local best="" best_version="" version
    for dir in "${candidates[@]}"; do
        [ -n "$dir" ] && [ -f "$dir/lib/cmake/Qt6/Qt6Config.cmake" ] || continue
        version=$("$dir/bin/qmake" -query QT_VERSION 2>/dev/null) || continue
        if [ -z "$best" ] || [ "$(printf '%s\n%s\n' "$best_version" "$version" | sort -V | tail -1)" = "$version" ]; then
            best=$dir
            best_version=$version
        fi
    done

    if [ -z "$best" ]; then
        echo "ERROR: no Qt6 installation found. Set QTDIR, or install Qt into one of:" >&2
        printf '  %s\n' "${QT_SEARCH_PATHS[@]}" >&2
        return 1
    fi
    printf '%s' "$best"
}

# $1: tool name, rest: directories to fall back to when it is not on PATH
add_tool_to_path() {
    local tool=$1 dir
    shift
    if command -v "$tool" >/dev/null 2>&1; then
        return 0
    fi
    for dir in "$@"; do
        if [ -x "$dir/$tool" ]; then
            export PATH="$dir:$PATH"
            return 0
        fi
    done
    echo "ERROR: $tool not found on PATH, nor in: $*" >&2
    return 1
}

if [ -z "${QTDIR:-}" ]; then
    QTDIR=$(find_qt) || return 1
    export QTDIR

    QT_ROOT=$(dirname "$(dirname "$QTDIR")")
    BREW_BIN="$(brew --prefix 2>/dev/null || echo /nonexistent)/bin"

    add_tool_to_path cmake "$QT_ROOT/Tools/CMake/CMake.app/Contents/bin" "$BREW_BIN" /Applications/CMake.app/Contents/bin || return 1
    add_tool_to_path ninja "$QT_ROOT/Tools/Ninja" "$BREW_BIN" || return 1

    export PATH="$QTDIR/bin:$PATH"

    echo -e "Qt path:\t$QTDIR"
    echo -e "CMake:\t\t$(command -v cmake)"
    echo -e "Ninja:\t\t$(command -v ninja)"
fi
