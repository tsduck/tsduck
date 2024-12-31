#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Bash completion for tsconfig.
#
#-----------------------------------------------------------------------------

__opts="--bin --cflags --config --help --include --java --lib --libs --plugin --prefix --python --so --static-libs --version --vernum"
[[ "$OSTYPE" == "linux-gnu"* ]] && __opts="$__opts --install-dvb-firmware"
complete -W "$__opts" tsconfig
unset __opts
