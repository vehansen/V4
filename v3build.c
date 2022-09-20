/*	V3BUILD.C - How to Build V3/V4 on various systems


**********************************************************************************************************************
On Linux System

==> v3ccdbg <==
cc -o dxv3 -m32 -w -g2 -pthread -DVSORT_AS_SUBROUTINE v4atomic.c v3driver.c v3pcku.c v3iou.c v3xctu.c v3oper.c v3operx.c v3edtu.c v3mscu.c v3sobu.c v3prsa.c v3prsb.c v3prsc.c v3prsd.c v3v4.c v3mthu.c v3stru.c v4hash.c v4is.c v4isu.c v4extern.c v4mm.c v4im.c v4imu.c v4eval.c v4dpi.c v4ctx.c vsort.c vcommon.c -lm -lc -lrt

==> v3ccopt <==
cc -o xv3 -m32 -w -pthread -O1 -DINHIBIT_WANTMYSQL -DINHIBIT_WANTORACLE -DVSORT_AS_SUBROUTINE v3driver.c v3pcku.c v3iou.c v3xctu.c v3oper.c v3operx.c v3edtu.c v3mscu.c v3sobu.c v3prsa.c v3prsb.c v3prsc.c v3prsd.c v3v4.c v3mthu.c v3stru.c v4hash.c v4is.c v4isu.c v4extern.c v4mm.c v4im.c v4imu.c v4eval.c v4dpi.c v4ctx.c vsort.c vcommon.c -lm -lc -lrt
chmod u+s xv3

==> v3ccopt1 <==
cc -o xv3 -m32 -w -pthread -O1 -DINHIBIT_MYSQL -DVSORT_AS_SUBROUTINE v3driver.c v3pcku.c v3iou.c v3xctu.c v3oper.c v3operx.c v3edtu.c v3mscu.c v3sobu.c v3prsa.c v3prsb.c v3prsc.c v3prsd.c v3v4.c v3mthu.c v3stru.c v4hash.c v4is.c v4isu.c v4extern.c v4mm.c v4im.c v4imu.c v4eval.c v4dpi.c v4ctx.c vsort.c vcommon.c -lm -lc
chmod u+s xv3

==> v4ccopt <==
Make sure tomcrypt stuff is up-to-date!!!!
cc -m32 -o xv4 -O3 -w -DLINUX486 -DLTC_NO_ROLC -DVSORT_AS_SUBROUTINE v4test.c v4hash.c v4is.c v4isu.c v4extern.c v4mm.c v4im.c v4imu.c v4eval.c v4dpi.c v4ctx.c vsort.c vcommon.c v4tc_sha1.c v4tc_sha256.c v4tc_crypt_argchk.c -lm -lc -l curl -L/usr/lib/mysql -l mysqlclient -lpthread
==> v4ccdbg <==
cc -m32 -o dxv4 -g2 -w -DLTC_NO_ROLC -DVSORT_AS_SUBROUTINE v4test.c v4hash.c v4is.c v4isu.c v4extern.c v4mm.c v4im.c v4imu.c v4eval.c v4dpi.c v4ctx.c vsort.c vcommon.c v4tc_sha1.c v4tc_sha256.c v4tc_crypt_argchk.c -lm -lc -l curl -L/usr/lib/mysql -l mysqlclient -l pthread

**********************************************************************************************************************
On Windows

See vlibCurl64.zip & vmysql64.zip - unzip into directories as referenced below (Note: top level does not need to be \vic\ - just be consistent)

Need the following includes -

d:\vic\curl64\include
d:\vic\mysql64\include


Need the following libs-

d:\vic\mysql64\lib\libmysql.lib
wsock32.lib
Psapi.lib
Wininet.lib
d:\vic\curl64\lib\libcurl.dll.a
Ws2_32.lib
crypt32.lib
kernel32.lib
user32.lib
gdi32.lib
winspool.lib
comdlg32.lib
advapi32.lib
shell32.lib
ole32.lib
oleaut32.lib
uuid.lib
Odbc32.lib

NOTE - need libmysql.dll in the same directory as xv4.exe !!!!!





**********************************************************************************************************************
On Raspberry PI

==> v4ccdbg-rp <==
cc -m32 -o dxv4 -g2 -w -DRASPPI -DLTC_NO_ROLC -DVSORT_AS_SUBROUTINE v4test.c v4hash.c v4is.c v4isu.c v4extern.c v4mm.c v4im.c v4imu.c v4eval.c v4dpi.c v4ctx.c vsort.c vcommon.c v4tc_sha1.c v4tc_sha256.c v4tc_crypt_argchk.c -lm -lc -l pthread

==> v4ccopt-rp <==
cc -m32 -o xv4-rp -O3 -w -DRASPPI -DLTC_NO_ROLC -DVSORT_AS_SUBROUTINE v4test.c v4hash.c v4is.c v4isu.c v4extern.c v4mm.c v4im.c v4imu.c v4eval.c v4dpi.c v4ctx.c vsort.c vcommon.c v4tc_sha1.c v4tc_sha256.c v4tc_crypt_argchk.c -lm -lc -l pthread

*/
