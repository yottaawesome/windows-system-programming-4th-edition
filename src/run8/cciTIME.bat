version >> cciTime.txt
RandFile 5000000 big.txt >> cciTime.txt
echo *************** >> cciTime.txt
echo cci >> cciTime.txt
timep cci 2 big.txt big_cc.txt >> cciTime.txt
del big_cc.txt >> cciTime.txt
echo *************** >> cciTime.txt
echo cciLBSS Y-LargeBuf Y-SeqScan N-FileSize >> cciTime.txt
timep cciLBSS 2 big.txt big_cc.txt >> cciTime.txt
del big_cc.txt >> cciTime.txt
echo *************** >> cciTime.txt
echo cciOV >> cciTime.txt
timep cciOV 2 big.txt big_cc.txt >> cciTime.txt
del big_cc.txt >> cciTime.txt
echo *************** >> cciTime.txt
echo cciEX >> cciTime.txt
timep cciEX 2 big.txt big_cc.txt >> cciTime.txt
echo *************** >> cciTime.txt
echo cciMM >> cciTime.txt
timep cciMM 2 big.txt big_cc.txt >> cciTime.txt
del big_cc.txt >> cciTime.txt
echo *************** >> cciTime.txt
echo cciMT >> cciTime.txt
timep cciMT 2 big.txt big_cc.txt >> cciTime.txt
del big_cc.txt >> cciTime.txt
echo *************** >> cciTime.txt
echo cciMTMM >> cciTime.txt
timep cciMTMM 2 big.txt big_cc.txt >> cciTime.txt
del big_cc.txt >> cciTime.txt
del big_cc.txt >> cciTime.txt
del big.txt >> cciTime.txt
echo *************** >> cciTime.txt
echo ****** Extra ****** Time the wc implementations, including the Cygwin version. >> cciTime.txt
echo ****** Create 8 64MB test files.
randfile 1000000 a.txt
randfile 1000000 b.txt
randfile 1000000 c.txt
randfile 1000000 d.txt
randfile 1000000 e.txt
randfile 1000000 f.txt
randfile 1000000 g.txt
randfile 1000000 h.txt
echo *************** >> cciTime.txt
echo wc >> cciTime.txt
timep wc a.txt b.txt c.txt d.txt e.txt f.txt g.txt h.txt >> cciTime.txt
echo *************** >> cciTime.txt
echo wcMT >> cciTime.txt
timep wcMT a.txt b.txt c.txt d.txt e.txt f.txt g.txt h.txt >> cciTime.txt
echo *************** >> cciTime.txt
echo wcMTMM >> cciTime.txt
timep wcMTMM a.txt b.txt c.txt d.txt e.txt f.txt g.txt h.txt >> cciTime.txt
echo *************** >> cciTime.txt
echo wcMT_VTP >> cciTime.txt
timep wcMT_VTP a.txt b.txt c.txt d.txt e.txt f.txt g.txt h.txt >> cciTime.txt
echo *************** >> cciTime.txt
del a.txt b.txt c.txt d.txt e.txt f.txt g.txt h.txt >> cciTime.txt
echo ===================================================== >> cciTime.txt
