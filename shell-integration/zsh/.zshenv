# Cotty zsh shell integration bootstrap.
# Adapted from Ghostty's zsh integration (GPLv3).
#
# This file is sourced automatically by zsh when ZDOTDIR points here.
# It restores the user's ZDOTDIR, sources their .zshenv, then loads
# the cotty-integration script for interactive shells.

# Disable PROMPT_SP — the '%' partial-line indicator for unterminated output.
# With shell integration active, OSC 133 markers handle prompt boundaries
# and the '%' indicator is redundant. Ghostty's integration also documents
# this interaction (ghostty-integration line 110-111).
'builtin' 'setopt' 'no_prompt_sp' 2>/dev/null

# Restore original ZDOTDIR (saved by Cotty before spawning zsh).
if [[ -n "${COTTY_ZSH_ZDOTDIR+X}" ]]; then
    'builtin' 'export' ZDOTDIR="$COTTY_ZSH_ZDOTDIR"
    'builtin' 'unset' 'COTTY_ZSH_ZDOTDIR'
else
    'builtin' 'unset' 'ZDOTDIR'
fi

# Source the user's real .zshenv (zsh treats unset ZDOTDIR as HOME).
{
    'builtin' 'typeset' _cotty_file=${ZDOTDIR-$HOME}"/.zshenv"
    [[ ! -r "$_cotty_file" ]] || 'builtin' 'source' '--' "$_cotty_file"
} always {
    # For interactive shells, load the integration script.
    if [[ -o 'interactive' ]]; then
        'builtin' 'typeset' _cotty_file="${${(%):-%x}:A:h}"/cotty-integration
        if [[ -r "$_cotty_file" ]]; then
            'builtin' 'autoload' '-Uz' '--' "$_cotty_file"
            "${_cotty_file:t}"
            'builtin' 'unfunction' '--' "${_cotty_file:t}"
        fi
    fi
    'builtin' 'unset' '_cotty_file'
}
