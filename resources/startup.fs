\
\ startup.fs, the default startup file for the 
\ forth environment...
\

\
\ set default dac values...
\
0 850 writeDAC
1 2097 writeDAC
2 3000 writeDAC
3 2048 writeDAC
4 850 writeDAC
5 2097 writeDAC
6 3000 writeDAC
7 1925 writeDAC
9 500 writeDAC
12 500 writeDAC

\
\ convenience routines...
\

\
\ run stf server...
\
: stf s" stf.sbi" find if fpga endif s" stfserv" find if exec endif ;

\
\ run stf menu program...
\
: menu s" stf.sbi" find if fpga endif s" menu" find if exec endif ;
