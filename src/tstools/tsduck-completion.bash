#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Bash completion for TSDuck commands.
#
#-----------------------------------------------------------------------------

# All TSDuck commands (automatically updated by makefile).
__ts_cmds=(tsanalyze tsbitrate tscharset tscmp tscrc32 tsdate tsdebug tsdektec tsdump tsecmg tseit tsemmg tsfclean tsfixcc tsftrunc tsfuzz tsgenecm tshides tslatencymonitor tslsdvb tsp tspacketize tspcap tspcontrol tspsi tsresync tsscan tssmartcard tsstuff tsswitch tstabcomp tstabdump tstables tsterinfo tstestecmg tsvatek tsversion tsxml)

# A filter to remove CR on Windows.
[[ $OSTYPE == cygwin || $OSTYPE == msys ]] && __ts_lines() { dos2unix; } || __ts_lines() { cat; }

# Assign COMPREPLY with the expansion of files or directories.
# Syntax: __ts_compgen_fd {-f|-d} current-value
__ts_compgen_fd()
{
    # Path expansion.
    local saved="$IFS"
    IFS=$'\n'
    COMPREPLY=($(compgen $1 -- "$2"))
    IFS="$saved"
    local i name
    for ((i=0; i<${#COMPREPLY[@]}; i++)); do
        name=${COMPREPLY[$i]//\\/\\\\}
        name=${name// /\\ }
        name=${name//\(/\\(}
        name=${name//)/\\)}
        name=${name//\[/\\[}
        name=${name//]/\\]}
        if [[ -d "${COMPREPLY[$i]}" ]]; then
            COMPREPLY[$i]="$name/"
        else
            COMPREPLY[$i]="$name "
        fi
    done
    compopt -o nospace 2>/dev/null
}

# Completion function for all TSDuck commands.
_tsduck()
{
    local cmd="$1"
    local curword="$2"
    local prevword="$3"
    local prevchar="${COMP_LINE:$(($COMP_POINT-1)):1}"

    # Filter spurious invocations.
    [[ -z $cmd ]] && return

    # If the current word is a variable reference, ignore command syntax, just expand variable names.
    if [[ $curword == \$* ]]; then
        COMPREPLY=($(compgen -W "$(env | sed -e '/^[A-Za-z0-9_]*=/!d' -e 's/=.*$//' -e 's/^/\\\$/')" -- "$curword"))
        return
    fi

    # All available options for this command.
    local cmdopts=$($cmd --help=options 2>/dev/null | __ts_lines | sed -e '/^@/d' -e '/:/!d' -e 's/:.*$//')

    # Check if previous option is a plugin introducer (ie. need a plugin name).
    if [[ $prevword == -I || $prevword == -P || $prevword == -O ]]; then
        if [[ $cmdopts == *$prevword* ]]; then
            # This type of plugin is supported by the command, get possible plugin names.
            COMPREPLY=($(compgen -W "$($cmd --list-plugins=names$prevword 2>/dev/null | __ts_lines)" -- "$curword"))
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
        if [[ $cmdopts == *$plopt* ]] && ($cmd --list-plugins=names$plopt | __ts_lines | grep -q "^$plname\$"); then
            # This plugin is supported by the command, use it as base command.
            cmd="$cmd $plopt $plname"
            cmdopts=$($cmd --help=options 2>/dev/null | __ts_lines | sed -e '/^@/d' -e '/:/!d' -e 's/:.*$//')
        else
            COMPREPLY=("{unknown-plugin$plopt-$plname}")
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
        COMPREPLY=($(compgen -W "$cmdopts" -- "$curword"))
        return
    elif [[ $prevword == -* ]]; then
        # Previous word is an option, current word is an option value or a parameter.
        opt="$prevword"
        val="$curword"
    else
        # Current word is a parameter.
        opt=@
        val="$curword"
    fi

    # Completion for an option value, check option validity.
    if [[ $opt != @ ]]; then
        case $(tr <<<$cmdopts ' ' '\n' | grep "^$opt" | wc -l | sed -e 's/ //g') in
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
    fi

    # Get syntax for that option.
    local syntax=$($cmd --help=options 2>/dev/null | __ts_lines | sed -e '/:/!d' -e "/^$opt/!d" -e 's/^[^:]*://')

    # If there is no possible value for that option, this is a parameter.
    # If the value is optional but not inside "--option=value", there is no value, this is a parameter.
    if [[ $syntax == bool* || ( $syntax == opt:* && -z $aftereq ) ]]; then
        opt=@
        syntax=$($cmd --help=options 2>/dev/null | __ts_lines | sed -e '/^@:/!d' -e 's/^@://')
    fi

    syntax=${syntax/#opt:/}
    case $syntax in
        enum:*)
            # Expand enumeration values.
            syntax=${syntax/#enum:/}
            COMPREPLY=($(compgen -W "${syntax//,/ }" -- "$val"))
            ;;
        file)
            # Expand files names or intermediate directories.
            __ts_compgen_fd -f "$val"
            ;;
        directory)
            # Expand directories (intermediate or final).
            __ts_compgen_fd -d "$val"
            ;;
        *)
            # Other types (eg. integers), no auto-completion possible.
            COMPREPLY=()
            ;;
    esac
}

# Declare completions for all TSDuck commands.
# Do not change the order of the following two lines, guess why...
[[ $OSTYPE == cygwin || $OSTYPE == msys ]] && complete -F _tsduck ${__ts_cmds[*]/%/.exe}
complete -F _tsduck ${__ts_cmds[*]}
