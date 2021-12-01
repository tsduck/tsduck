#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2021, Thierry Lelegard
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#
#-----------------------------------------------------------------------------
#
#  Bash completion for TSDuck commands.
#
#-----------------------------------------------------------------------------

# Completion function for all TSDuck commands.
_tsduck()
{
    local cmd="$1"
    local curword="$2"
    local prevword="$3"
    local prevchar="${COMP_LINE:$(($COMP_POINT-1)):1}"

    # All available options:types for this command.
    local opts=$($cmd --help=options 2>&1 | sed -e 's/:.*$//')

    # Check if previous option is a plugin introducer (ie. need a plugin name).
    if [[ $prevword == -I || $prevword == -P || $prevword == -O ]]; then
        if [[ $opts == *$prevword* ]]; then
            # This type of plugin is supported by the command, get possible plugin names.
            COMPREPLY=($($cmd --list-plugins=${prevword/-/} 2>&1 | sed -e '/:/!d' -e 's/:.*//' -e "/^$curword/!d"))
            return
        else
            COMPREPLY=("{unknown-option$prevword}")
            return
        fi
    fi

    # Get the last plugin introducer, if any (ie.check if we are in a plugin argument list).
    local plopt plname opt i
    for ((i=$(($COMP_CWORD-1)); $i>0; i--)); do
        opt="${COMP_WORDS[$i]}"
        if [[ $opt == -I || $opt == -P || $opt == -O ]]; then
            plopt=$opt
            plname=${COMP_WORDS[$(($i+1))]}
            break
        fi
    done
    if [[ -n $plopt && -n $plname ]]; then
        # We are in a plugin argument list.
        if [[ $opts == *$plopt* ]] && ($cmd --list-plugins=${plopt/-/} 2>&1 | grep -q "^$plname:"); then
            # This plugin is supported by the command, use it as base command.
            cmd="$cmd $plopt $plname"
        else
            COMPREPLY=("{unknown-plugin-$plname}")
            return
        fi
    fi

    # Check if we are in an option, the value of an option, etc.
    local val aftereq
    if [[ $prevword == -* && $prevchar == = && -z $curword ]]; then
        # At end of "--option="
        aftereq=true
        opt="$prevword"
        val=
    elif [[ $COMP_CWORD -gt 2 && ${COMP_WORDS[$(($COMP_CWORD-2))]} == -* && $prevword == = ]]; then
        # At end of "--option=val"
        aftereq=true
        opt="${COMP_WORDS[$(($COMP_CWORD-2))]}"
        val="$curword"
    elif [[ $curword == -* ]]; then
        # Current word is an option name without value, get all possible matching options.
        COMPREPLY=($($cmd --help=options 2>&1 | sed -e 's/:.*$//' -e "/^$curword/!d"))
        return
    elif [[ $prevword == -* ]]; then
        # Previous word is an option.
        opt="$prevword"
        val="$curword"
    else
        # Current word is some parameter, use default completion.
        return
    fi

    # Completion for an option value, check option validity.
    case $($cmd --help=options 2>&1 | grep "^$opt" | wc -l) in
        1)  # Just one possible option, continue later.
            ;;
        0)  # Unknown option.
            COMPREPLY=("{unknown-option$opt}")
            return
            ;;
        *)  # Several options match that prefix.
            COMPREPLY=("{ambiguous-option$opt}")
            return
            ;;
    esac

    # Get syntax for that option.
    local syntax=$($cmd --help=options 2>&1 | sed -e '/:/!d' -e "/^$opt/!d" -e 's/^[^:]*://')

    # If there is no possible value for that option, this is another parameter.
    [[ -z $syntax ]] && return

    # If the value is optional but not inside "--option=value", there is no value.
    [[ $syntax == opt:* && -z $aftereq ]] && return
    syntax=${syntax/#opt:/}

    # Expand enumerations only for now.
    if [[ $syntax == enum:* ]]; then
        COMPREPLY=($(tr <<<${syntax/#enum:/} ',' '\n' | grep "^$val"))
    fi
}

# Declare a completion function for a command.
__ts_complete()
{
    complete -o bashdefault -o default -o nospace -F _tsduck $1 2>/dev/null || complete -o default -o nospace -F _tsduck $1
}

# All TSDuck commands (automatically updated by makefile).
__ts_cmds=(tsanalyze tsbitrate tscharset tscmp tsdate tsdektec tsdump tsecmg tseit tsemmg tsfclean tsfixcc tsftrunc tsgenecm tshides tslsdvb tsp tspacketize tspcap tspcontrol tspsi tsresync tsscan tssmartcard tsstuff tsswitch tstabcomp tstabdump tstables tsterinfo tsversion tsxml)

# Declare completions for all TSDuck commands.
for __cmd in ${__ts_cmds[*]}; do
    __ts_complete $__cmd
    [ "$OSTYPE" = cygwin ] && __ts_complete $__cmd.exe
done
unset __cmd __ts_cmds __ts_complete
