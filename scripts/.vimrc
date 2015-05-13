set nocompatible
filetype plugin on
set term=ansi
set title
set number
syntax on
hi Comment term=none ctermfg=green ctermbg=darkgray guifg=Gray
se hlsearch
" Ctrl-L clears the highlight from the last search
noremap <C-l> :nohlsearch<CR><C-l>
noremap! <C-l> <ESC>:nohlsearch<CR><C-l>
autocmd! BufNewFile,BufRead *.ino setlocal ft=arduino
