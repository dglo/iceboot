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
\ run stf server...
\
: stf s" stf.sbi" find if fpga endif s" stfserv" find if exec endif ;

\
\ run stf menu program...
\
: menu s" stf.sbi" find if fpga endif s" menu" find if exec endif ;

\
\ run domapp
\
: domapp s" domapp.sbi" find if fpga endif s" domapp" find if exec endif ;

\
\ comm stuff...
\
$90081030 constant comctl
$90081034 constant comstatus
$90081038 constant comtx
$9008103c constant comrx

$50000000 constant CPLD

s" iceboot.sbi" find if fpga drop $01200000 comctl ! endif

: yorn if s" yes" else s" no" endif type crlf type ;

: rx-ready comstatus @ 1 and s" rx-ready: " type yorn ;
: rx-empty comstatus @ 2 and s" rx-empty: " type yorn ;
: rx-almost-full comstatus @ $40 and s" rx-almost-full: " type yorn ;
: rx-full comstatus @ $80 and s" rx-full: " type yorn ;
: rx-count comstatus @ $ff00 and 8 rshift s" rx-count: " type . drop ;
: tx-almost-empty comstatus @ $00010000 and s" tx-almost-empty: " type yorn ;
: tx-almost-full comstatus @ $00020000 and s" tx-almost-full: " type yorn ;
: tx-read-empty comstatus @ $00100000 and s" tx-read-empty: " type yorn ;

: prt-status rx-ready rx-empty rx-almost-full rx-full rx-count tx-almost-empty tx-almost-full tx-read-empty ;

: set-rx-done 1 comctl ! ;

: prt-err base @ comstatus @ &24 rshift &16 base ! . swap base ! drop ;

: echo rcv swap dup constant buf swap send buf free drop ;

: not 1 and 1 xor ;
: serial-power CPLD $b + c@ 1 and ;

\
\ random number generator...
\
: rand1 69069 * 1 + dup addr i 4 * + ! ;
: rand-alloc dup 4 * allocate drop constant addr ;
: rand rand-alloc dup constant len 0 ?DO rand1 LOOP drop addr len 4 * ;

\
\ echo back a packet, incoming packet should be seed (4 bytes), i
\ len (4 bytes), ...
\ outgoing packet is a random packet of len 32 bit words, consisting
\ of: X(n+1) = X(n)*69069 + 1
\ where X(0) <- seed
\
: echo-mk-pkt rcv drop dup @ swap dup 4 + @ swap free drop rand ; 
: echo-pkt echo-mk-pkt send addr free drop ;

: echo-mode 2000000000 0 ?DO echo LOOP ;
: echo-pkt-mode 2000000000 0 ?DO echo-pkt LOOP ;

\ $11223344 $90081058 !
\ $15566 $9008105c ! 

\ serial-power not if echo-mode endif

