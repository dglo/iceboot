: wait 200000 usleep
: wait20ms 20000 usleep
: wait1ms 1000 usleep
: spel $90081010 @ . drop
: spef $90081020 @ . drop
: disccount10ms $100 $90081060 !

: spefscanmessg s" spefscanmessg" type crlf type ;
: spefscan spefscanmessg disccount10ms 570 470 ?DO 9 i writeDAC  i . drop wait20ms spef LOOP
: spefscanlong spefscanmessg 570 470 ?DO 9 i writeDAC  i . drop wait spef LOOP
: spelscanmessg s" spelscanmessg" type crlf type ;
: spelscan spelscanmessg disccount10ms 570 470 ?DO 9 i writeDAC  i . drop wait20ms spel LOOP
: spelscanlong spelscanmessg 570 470 ?DO 9 i writeDAC  i . drop wait spel LOOP


: atwdatr                $1 $90081000 !  
: atwdbtr              $100 $90081000 !
: atwdtr               $101 $90081000 !

: atwdatrfe        $1000001 $90081000 !  
: atwdbtrfe        $1000100 $90081000 !
: atwdtrfe         $1000101 $90081000 !

: atwdatrledon    $04000001 $90081000 !
: atwdbtrledon    $04000100 $90081000 !
: atwdtrledon     $04000101 $90081000 !

: atwdatrspeled    $04000002 $90081000 !
: atwdbtrspeled    $04000200 $90081000 !
: atwdtrspeled     $04000202 $90081000 !

: atwdaledtr      $04000008 $90081000 !
: atwdbledtr      $04000800 $90081000 !
: atwdledtr       $04000808 $90081000 !

: atwdatrspefe     $1000002 $90081000 !
: atwdbtrspefe     $1000200 $90081000 !
: atwdtrspefe      $1000202 $90081000 !

: atwdatrspe        $000002 $90081000 !
: atwdbtrspe        $000200 $90081000 !
: atwdtrspe         $000202 $90081000 !

: atwdafadctrspe        $020002 $90081000 !
: atwdbfadctrspe        $020200 $90081000 !
: atwdfadctrspe         $020202 $90081000 !

: atwdtrr2r       $40000101 $90081000 !
: atwdatrr2r      $40000001 $90081000 !
: atwdbtrr2r      $40000100 $90081000 !

: atwdtrr2rch3    $10000101 $90081000 !
: atwdatrr2rch3   $10000001 $90081000 !
: atwdbtrr2rch3   $10000100 $90081000 !

: atwdard $90084000 512 od
: atwdbrd $90085000 512 od

: atwdrs              $0 $90081000 !
: atwdrsfe      $1000000 $90081000 !
: atwdrsled    $04000000 $90081000 !
: atwdrsr2r    $40000000 $90081000 !
: atwdrsr2rch3 $10000000 $90081000 !

: atall  atwdrs atwdtr  wait1ms atwdard atwdbrd atwdrs
: ataall atwdrs atwdatr wait1ms atwdard         atwdrs
: atball atwdrs atwdbtr wait1ms atwdbrd atwdrs

: atallfe   atwdrsfe atwdtrfe  wait1ms atwdard atwdbrd atwdrsfe
: ataallfe  atwdrsfe atwdatrfe wait1ms atwdard         atwdrsfe 
: atballfe  atwdrsfe atwdbtrfe wait1ms atwdbrd         atwdrsfe 

: atallled   atwdrsled atwdledtr wait1ms atwdard atwdbrd atwdrsled
: ataallled  atwdrsled atwdaledtr wait1ms atwdard         atwdrsled
: atballled  atwdrsled atwdbledtr wait1ms         atwdbrd atwdrsled

: atallspeled   atwdrsled atwdtrspeled  wait1ms atwdard atwdbrd atwdrsled
: ataallspeled  atwdrsled atwdatrspeled wait1ms atwdard         atwdrsled
: atballspeled  atwdrsled atwdbtrspeled wait1ms         atwdbrd atwdrsled

: atallspefe  atwdrsfe atwdtrspefe  wait1ms atwdard atwdbrd atwdrsfe
: ataallspefe atwdrsfe atwdatrspefe wait1ms atwdard         atwdrsfe
: atballspefe atwdrsfe atwdbtrspefe wait1ms         atwdbrd atwdrsfe

: atallspe  atwdrs atwdtrspe  wait1ms atwdard atwdbrd atwdrs
: ataallspe atwdrs atwdatrspe wait1ms atwdard         atwdrs
: atballspe atwdrs atwdbtrspe wait1ms         atwdbrd atwdrs

: atallfadcspe  atwdrs atwdfadctrspe  wait1ms atwdard atwdbrd fadcrd atwdrs
: ataallfadcspe atwdrs atwdafadctrspe wait1ms atwdard         fadcrd atwdrs
: atballfadcspe atwdrs atwdbfadctrspe wait1ms         atwdbrd fadcrd atwdrs

: atallr2r   atwdrs atwdtrr2r  wait1ms atwdard atwdbrd atwdrs
: ataallr2r  atwdrs atwdatrr2r wait1ms atwdard         atwdrs
: atballr2r  atwdrs atwdbtrr2r wait1ms         atwdbrd atwdrs

: atallr2ron   atwdrsr2r atwdtrr2r  wait1ms atwdard atwdbrd atwdrsr2r
: ataallr2ron  atwdrsr2r atwdatrr2r wait1ms atwdard         atwdrsr2r
: atballr2ron  atwdrsr2r atwdbtrr2r wait1ms         atwdbrd atwdrsr2r

: atallr2rch3on  atwdrsr2rch3 atwdtrr2rch3  wait1ms atwdard atwdbrd atwdrsr2rch3
: ataallr2rch3on atwdrsr2rch3 atwdatrr2rch3  wait1ms atwdard         atwdrsr2rch3
: atballr2rch3on atwdrsr2rch3 atwdbtrr2rch3  wait1ms         atwdbrd atwdrsr2rch3
 
: clock1xmuxmessg s" clock1xmuxmessg" type crlf type ;
: clock1xmux clock1xmuxmessg 0 analogMuxInput
: flashermuxmessg s" flashermuxmessg" type crlf type ;
: flashermux  flashermuxmessg  3 analogMuxInput
: cominmuxmessg s" cominmuxmessg" type crlf type ;
: cominmux  cominmuxmessg  6 analogMuxInput
: ledmuxmessg s" ledmuxmessg" type crlf type ;   
: ledmux   ledmuxmessg   2 analogMuxInput
: clock2xmuxmessg s" clock2xmuxmessg" type crlf type ; 
: clock2xmux clock2xmuxmessg 1 analogMuxInput

: pulseron0messg s" pulseron0messg" type crlf type ; 
: pulseron10messg s" pulseron10messg" type crlf type ; 
: pulseron30messg s" pulseron30messg" type crlf type ; 
: pulseron60messg s" pulseron60messg" type crlf type ; 
: pulseron120messg s" pulseron120messg" type crlf type ; 
: pulseron900messg s" pulseron900messg" type crlf type ; 
: pulseroffmessg s" pulseroffmessg" type crlf type ; 

: pulseron0   pulseron0messg     11 0   writeDAC 0 $90081000 ! $1000000 $90081000 !
: pulseron10  pulseron10messg    11 10  writeDAC 0 $90081000 ! $1000000 $90081000 !
: pulseron30  pulseron30messg    11 30  writeDAC 0 $90081000 ! $1000000 $90081000 !
: pulseron60  pulseron60messg    11 60  writeDAC 0 $90081000 ! $1000000 $90081000 !
: pulseron120 pulseron120messg   11 120 writeDAC 0 $90081000 ! $1000000 $90081000 !
: pulseron900 pulseron900messg   11 900 writeDAC 0 $90081000 ! $1000000 $90081000 !
: pulseroff   pulseroffmessg     11 0   writeDAC 0 $90081000 !

: ledonmessg s" ledonmessg" type crlf type ; 
: ledoffmessg s" ledoffmessg" type crlf type ;

: enableLED  CPLD 9 + c@ 8 or      CPLD 9 + c!
: disableLED CPLD 9 + c@ 8 not and CPLD 9 + c!
: ledon  ledonmessg  enableLED  0 $90081000 ! $04000000 $90081000 !
: ledoff ledoffmessg disableLED 0 $90081000 !
: ledmax  12 0    writeDAC 
: ledmin  12 1023 writeDAC


: fadctrfe      $1010000 $90081000 !
: fadctrspefe   $1020000 $90081000 !
: fadctr          $10000 $90081000 !
: fadctrspe       $20000 $90081000 !
: fadctrr2r    $40010000 $90081000 !
: fadctrr2rch3 $10010000 $90081000 !
: fadctrspeled $04020000 $90081000 !

: fadcrd $90083000 512 od

: fadcrsfe $1000000 $90081000 !
: fadcrs   0        $90081000 !
: fadcrsr2r $10000000  $90081000 !

: fadcall        fadcrs    fadctr       wait1ms fadcrd fadcrs
: fadcallfe      fadcrsfe  fadctrfe     wait1ms fadcrd fadcrsfe
: fadcallfesync  fadcrs    fadctrfe     wait1ms fadcrd fadcrs
: fadcallspefe   fadcrsfe  fadctrspefe  wait1ms fadcrd fadcrsfe
: fadcallr2r     fadcrs    fadctrr2r    wait1ms fadcrd fadcrs
: fadcallspeled  fadcrsled fadctrspeled wait1ms fadcrd fadcrsled

: comdactria  $0 $90081008 ! $1 $90081008 !
: comadctr    $10 $90081008 !
: comadctrtria  $11 $90081008 !
: comadcrd $90082000 512 od
: comadcrs      $0 $90081008 !
: comadcrstria  $1 $90081008 !
: comadcall       comadcrs comadctr wait1ms comadcrd comadcrs
: comadcalltria   comadcrstria comadctrtria wait1ms comadcrd comadcrstria
: comadcalltrtria comadcrs comadctrtria wait1ms comadcrd comadcrs

: comadconloopmssg s" comadconloopmssg" type crlf type ;
: comadcoffloopmessg s" comadcoffloopmessg" type crlf type ;
: comadctrtrialoop comadconloopmssg   100 0 ?DO comadcalltrtria LOOP
: comadctrialoop   comadconloopmssg   100 0 ?DO comadcalltria LOOP
: comadcloop       comadcoffloopmessg 100 0 ?DO comadcall LOOP

: fadcloop   10 0 ?DO fadcall   LOOP
: fadcfeloop 10 0 ?DO fadcallfe LOOP
: fadcspefeloop   9 520 writeDAC 10 0 ?DO fadcallspefe LOOP
: fadcspeledloop  9 520 writeDAC 10 0 ?DO fadcallspeled LOOP

: atwdloopmessg s" atwdloopmessg" type crlf type ;
: atwdloopshortmessg s" atwdloopshortmessg" type crlf type ;
: atwdfeloopmessg s" atwdfeloopmessg" type crlf type ;
: atwdledloopmessg s" atwdledloopmessg" type crlf type ;
: atwdspeledloopmessg s" atwdspeledloopmessg" type crlf type ;
: atwdspefeloopmessg s" atwdspefeloopmessg" type crlf type ;
: atwdspeloopmessg s" atwdspeloopmessg" type crlf type ;
: atwdfadcspeloopmessg s" atwdfadcspeloopmessg" type crlf type ;

: atwdloop        atwdloopmessg                       100 0 ?DO ataall        atball        LOOP
: atwdloopshort   atwdloopshortmessg                  10  0 ?DO ataall        atball        LOOP
: atwdfeloop      atwdfeloopmessg                     100 0 ?DO ataallfe      atballfe      LOOP
: atwdledloop     atwdledloopmessg                    100 0 ?DO ataallled     atballled     LOOP
: atwdspeledloop  atwdspeledloopmessg  9 520 writeDAC 100 0 ?DO ataallspeled  atballspeled  LOOP
: atwdspefeloop   atwdspefeloopmessg   9 520 writeDAC 100 0 ?DO ataallspefe   atballspefe   LOOP
: atwdspeloop     atwdspeloopmessg     9 520 writeDAC 100 0 ?DO ataallspe     atballspe     LOOP
: atwdfadcspeloop atwdfadcspeloopmessg 9 520 writeDAC 100 0 ?DO ataallfadcspe atballfadcspe LOOP

: pingpongmessg s" pingpongmessg"  type crlf type ;
: pingpong pingpongmessg  9 520 writeDAC 0 $f acq-pp $01000000 208000 od
 
: tempboard readTemp prtTemp
: pedestalmessg s" pedestalmessg" type crlf type ;
: pedestal pedestalmessg 50 0 ?DO 7 i 100 * writeDAC ataall atball ataall atball LOOP  7 1925 writeDAC
: linearitymessg s" linearitymessg"  type crlf type ;
: linearity linearitymessg pulseron0  9 520 writeDAC 33 1 ?DO 11 i 30 * writeDAC ataallspefe atballspefe ataallspefe atballspefe LOOP pulseroff
: adcsmessg s" adcsmessg"  type crlf type ;
: adcs adcsmessg 8 0 ?DO i . drop i readADC . drop LOOP
: looptempmessg s" looptempmessg" type crlf type ;
: looptemp looptempmessg 100 0 ?DO i . drop 1000000 usleep tempboard LOOP

: turnonbasemessg s" turnonbasemessg" type crlf type ;
: turnoffbasemessg s" turnoffbasemessg" type crlf type ;
: sethvmessg s" sethvmessg" type crlf type ;
: sethvto0messg s" sethvto0messg" type crlf type ;
: pmtonmessg s" pmtonmessg" type crlf type ;
: pmtoffmessg s" pmtoffmessg" type crlf type ;

: turnonbase turnonbasemessg   $1 $50000009 c!
: turnoffbase turnoffbasemessg $0 $50000009 c!
: sethv     sethvmessg      3400 writeActiveBaseDAC
: sethvto0     sethvto0messg      0 writeActiveBaseDAC
: pmton   pmtonmessg turnonbase sethv 2000000 usleep readBaseADC .
: pmtoff pmtoffmessg sethvto0  turnoffbase 2000000 usleep readBaseADC .

: lcrs         $0         $90081018 !
: lcactlatch   $0000f000  $90081018 !
: lcrd      &16 base ! $9008101c @ . drop &10 base ! 
: lctxdwhi     $0000f100  $90081018 !
: lctxdwlo     $0000f200  $90081018 !
: lctxuphi     $0000f400  $90081018 !
: lctxuplo     $0000f800  $90081018 !
: lctxhihi     $0000f500  $90081018 !
: lctxhilo     $0000f900  $90081018 !
: lctxlolo     $0000fA00  $90081018 !
: lctxlohi     $0000f600  $90081018 !

: lcrsff         $4         $90081018 !
: lcactlatchff   $0000f004  $90081018 !
: lcrdff      &16 base ! $9008101c @ . drop &10 base ! 
: lctxdwhiff     $0000f104  $90081018 !
: lctxdwloff     $0000f204  $90081018 !
: lctxuphiff     $0000f404  $90081018 !
: lctxuploff     $0000f804  $90081018 !
: lctxhihiff     $0000f504  $90081018 !
: lctxhiloff     $0000f904  $90081018 !
: lctxloloff     $0000fA04  $90081018 !
: lctxlohiff     $0000f604  $90081018 !

: lcmessage  s" lctestsmessg" type crlf type ;
: lctestone  lcrs lcrd lcrs lctxdwhi lcrd lcrs lctxdwlo lcrd lcrs lctxuphi lcrd lcrs lctxuplo lcrd 
: lctestboth lcrs lctxhihi lcrd lcrs lctxhilo lcrd lcrs lctxlolo lcrd lcrs lctxlohi lcrd 
: lctestoneff  lcrsff lcrdff lcrsff lctxdwhiff lcrdff lcrsff lctxdwloff lcrdff lcrsff lctxuphiff lcrdff lcrsff lctxuploff lcrdff 
: lctestbothff lcrsff lctxhihiff lcrdff lcrsff lctxhiloff lcrdff lcrsff lctxloloff lcrdff lcrsff lctxlohiff lcrdff 
: lcresults 1100aa00 11009a00 11006a00 1100a900 1100a600 11009900 11009600 11006600 11006900


0 850 writeDAC 1 2097 writeDAC 2 600 writeDAC 3 2048 writeDAC
4 850 writeDAC 5 2097 writeDAC 6 600 writeDAC 7 1925 writeDAC
10 700 writeDAC
13 800 writeDAC
14 1023 writeDAC
15 1023 writeDAC

