\
\ startup.fs, the default startup file for the 
\ forth environment...
\

\
\ set default dac values...
\
0 850 writeDAC
1 2097 writeDAC
2 600 writeDAC
3 2048 writeDAC
4 850 writeDAC
5 2097 writeDAC
6 600 writeDAC
7 1925 writeDAC
9 500 writeDAC
10 700 writeDAC
12 1023 writeDAC
13 800 writeDAC
14 1023 writeDAC
15 1023 writeDAC

\
\ run stf server...
\
: stf s" iceboot.sbi" find if fpga endif s" stfserv" find if exec endif ;

\
\ run domapp
\
: testdomapp s" testdomapp" find if exec endif ;
: domapp-sbi s" domapp.sbi" find if fpga endif ;
: domapp domapp-sbi s" domapp" find if exec endif ;
: domapp-test domapp-sbi s" domapp-test" find if exec endif ;
\
\ run echomode
: echo-mode s" echomode" find if exec endif ;
: echo-mode-cb s" configboot.sbi" find if fpga echo-mode endif ;

\
\ comm stuff...
\
$90081030 constant comctl
$90081034 constant comstatus
$90081038 constant comtx
$9008103c constant comrx

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
: comm-avail comstatus @ $00000008 and s" comm-avail: " type yorn ;

: prt-status-0 rx-ready rx-empty rx-almost-full rx-full ;
: prt-status-1 rx-count tx-almost-empty tx-almost-full ;
: prt-status-2 tx-read-empty comm-avail ;
: prt-status prt-status-0 prt-status-1 prt-status-2 ;

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

: echo-pkt-mode 2000000000 0 ?DO echo-pkt LOOP ;

\ $11223344 $90081058 !
\ $15566 $9008105c ! 

\ serial-power not if echo-mode endif

CPLD $f + c@ constant boot-status

: prt-init-done boot-status $4 and s" init_done: " type yorn ;
: prt-n-config boot-status $8 and s" nCONFIG: " type yorn ;
: prt-conf-done boot-status $10 and s" conf_done: " type yorn ;
: prt-n-por boot-status $20 and s" nPOR: " type yorn ;
: prt-n-reset boot-status $40 and s" nRESET: " type yorn ;
: prt-comm-reset boot-status $80 and s" comm-reset: " type yorn ; 
: prt-boot-status-0 prt-init-done prt-n-config prt-conf-done ;
: prt-boot-status-1 prt-n-por prt-n-reset prt-comm-reset ;
: prt-boot-status prt-boot-status-0 prt-boot-status-1 ;

: no-comm s" stf-nocomm.sbi" find if fpga endif ;
: wiggle s" wiggle" find if exec endif ;

