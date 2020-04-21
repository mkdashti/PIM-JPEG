#!/bin/bash

SCRIPT_DIR="$(readlink -f "$(dirname "${BASH_SOURCE[0]}")")"

export UPMEM_HOME="${SCRIPT_DIR}"
echo "Setting UPMEM_HOME to ${UPMEM_HOME} and updating PATH/LD_LIBRARY_PATH"
export LD_LIBRARY_PATH="${UPMEM_HOME}/lib${LD_LIBRARY_PATH+:$LD_LIBRARY_PATH}"
export PATH="${UPMEM_HOME}/bin:${PATH}"

_DEFAULT_BACKEND=${1:-simulator}
echo "Setting default backend to ${_DEFAULT_BACKEND} in UPMEM_PROFILE_BASE"
export UPMEM_PROFILE_BASE=backend=${_DEFAULT_BACKEND}
