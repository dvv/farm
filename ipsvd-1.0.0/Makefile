IT=tcpsvd udpsvd ipsvd-cdb

default: command.ssl $(IT)
	$(MAKE) `cat command.ssl`

check: check-tcpsvd check-udpsvd check-ipsvd-cdb
	./check-local $(IT) `grep -v nossl <command.ssl || :`

tcpsvd: load tcpsvd.o ipsvd_log.o ipsvd_fmt.o ipsvd_check.o ipsvd_hostname.o \
	  ipsvd_phcc.o ipsvd_scan.o uidgid.o cdb.a dns.a unix.a byte.a \
	  time.a socket.lib
	./load tcpsvd ipsvd_log.o ipsvd_fmt.o ipsvd_check.o ipsvd_hostname.o \
	  ipsvd_phcc.o ipsvd_scan.o uidgid.o cdb.a dns.a unix.a byte.a \
	  time.a `cat socket.lib`

sslsvd: load sslsvd.o ssl_io.o sslerror_str.o ipsvd_log.o ipsvd_fmt.o \
	  ipsvd_check.o ipsvd_hostname.o ipsvd_phcc.o ipsvd_scan.o uidgid.o \
	  cdb.a dns.a unix.a byte.a time.a socket.lib matrixssl.a
	./load sslsvd ssl_io.o sslerror_str.o ipsvd_log.o ipsvd_fmt.o \
	  ipsvd_check.o ipsvd_hostname.o ipsvd_phcc.o ipsvd_scan.o uidgid.o \
	  cdb.a dns.a unix.a byte.a time.a `cat socket.lib` matrixssl.a

udpsvd: load udpsvd.o ipsvd_log.o ipsvd_fmt.o ipsvd_check.o ipsvd_hostname.o \
	  ipsvd_scan.o uidgid.o cdb.a dns.a unix.a byte.a time.a socket.lib
	./load udpsvd ipsvd_log.o ipsvd_fmt.o ipsvd_check.o ipsvd_hostname.o \
	  ipsvd_scan.o uidgid.o cdb.a dns.a unix.a byte.a time.a \
	  `cat socket.lib`

ipsvd-cdb: load ipsvd-cdb.o ipsvd_fmt.o ipsvd_check.o cdb.a dns.a unix.a \
	  byte.a time.a
	./load ipsvd-cdb ipsvd_fmt.o ipsvd_check.o cdb.a dns.a unix.a byte.a \
	  time.a `cat socket.lib`

sslio: load sslio.o ssl_io.o uidgid.o sslerror_str.o unix.a byte.a time.a \
	  matrixssl.a
	./load sslio ssl_io.o uidgid.o sslerror_str.o unix.a byte.a time.a \
	  matrixssl.a

matrixssl.a: makelib matrixssl
	install -m0644 Makefile.matrixssl matrixssl/src/Makefile.matrixssl
	(cd matrixssl/src && $(MAKE) -fMakefile.matrixssl matrixssl.a)
	install -m0644 matrixssl/src/matrixssl.a matrixssl.a

matrixssl: matrixssl.tar.gz compile load
	gzip -dc matrixssl.tar.gz |tar xf -
	test -e matrixssl || ln -s matrixssl-* matrixssl
	sed -e 's/#define USE_MULTITHREADING/#undef USE_MULTITHREADING/' \
	  <matrixssl/src/matrixConfig.h >matrixConfig.h && \
	    mv -f matrixConfig.h matrixssl/src/matrixConfig.h
	echo '#ifndef CLK_TCK' >>matrixssl/src/os/osLayer.h
	echo '#define CLK_TCK CLOCKS_PER_SEC' >>matrixssl/src/os/osLayer.h
	echo '#endif' >>matrixssl/src/os/osLayer.h
	touch matrixssl

check-tcpsvd: load check-tcpsvd.o unix.a byte.a socket.lib
	./load check-tcpsvd unix.a byte.a `cat socket.lib`

check-udpsvd: load check-udpsvd.o unix.a byte.a socket.lib
	./load check-udpsvd unix.a byte.a `cat socket.lib`

check-ipsvd-cdb: load check-ipsvd-cdb.o uint32_unpack.o unix.a byte.a
	./load check-ipsvd-cdb uint32_unpack.o unix.a byte.a

tcpsvd.o: compile sysdeps tcpsvd.c ipsvd_log.h
	./compile tcpsvd.c

sslsvd.o: compile sysdeps tcpsvd.c ssl_io.c ipsvd_log.h matrixssl
	./compile -I./matrixssl sslsvd.c

ssl_io.o: compile sysdeps ssl_io.c matrixssl
	./compile -I./matrixssl ssl_io.c

udpsvd.o: compile sysdeps udpsvd.c ipsvd_log.h
	./compile udpsvd.c

ipsvd-cdb.o: compile sysdeps ipsvd-cdb.c
	./compile ipsvd-cdb.c

sslio.o: compile sysdeps sslio.c matrixssl
	./compile -I./matrixssl sslio.c

check-tcpsvd.o: compile check-tcpsvd.c
	./compile check-tcpsvd.c

check-udpsvd.o: compile check-udpsvd.c
	./compile check-udpsvd.c

check-ipsvd-cdb.o: compile uint32.h check-ipsvd-cdb.c
	./compile check-ipsvd-cdb.c

ipsvd_log.o: compile ipsvd_log.c
	./compile ipsvd_log.c

ipsvd_fmt.o: compile ipsvd_fmt.c
	./compile ipsvd_fmt.c

ipsvd_check.o: compile ipsvd_check.c uint64.h
	./compile ipsvd_check.c

ipsvd_hostname.o: compile ipsvd_hostname.c uint64.h
	./compile ipsvd_hostname.c

ipsvd_phcc.o: compile ipsvd_phcc.c
	./compile ipsvd_phcc.c

ipsvd_scan.o: compile ipsvd_scan.c
	./compile ipsvd_scan.c

uidgid.o: compile uidgid.c uidgid.h
	./compile uidgid.c

sslerror_str.o: compile sslerror_str.c sslerror_str.h matrixssl
	./compile -I./matrixssl sslerror_str.c

socket.lib: compile load trysocketlib.c
	./compile trysocketlib.c
	(./load trysocketlib >/dev/null 2>&1 || \
	  (./load trysocketlib -lxnet >/dev/null 2>&1 && echo '-lxnet') || \
	  (./load trysocketlib -lsocket -lnsl >/dev/null 2>&1 && \
	    echo '-lsocket -lnsl') ) >socket.lib
	rm -f trysocketlib.o trysocketlib

command.ssl: command.sslcheck
command.sslcheck:
	( (test ! -r matrixssl.tar.gz && echo nossl) || \
	  echo sslio sslsvd) >command.sslcheck
	mv -f command.sslcheck command.ssl

nossl:
	: no ssl library, not making sslio

clean:
	rm -f `cat TARGETS` *~
	rm -rf matrixssl matrixssl-*


alloc.o: alloc.c alloc.h compile error.h
	./compile alloc.c

alloc_re.o: alloc.h alloc_re.c byte.h compile
	./compile alloc_re.c

buffer.o: buffer.c buffer.h compile
	./compile buffer.c

buffer_0.o: buffer.h buffer_0.c compile
	./compile buffer_0.c

buffer_1.o: buffer.h buffer_1.c compile
	./compile buffer_1.c

buffer_2.o: buffer.h buffer_2.c compile
	./compile buffer_2.c

buffer_get.o: buffer.h buffer_get.c byte.h compile error.h
	./compile buffer_get.c

buffer_put.o: buffer.h buffer_put.c byte.h compile error.h str.h
	./compile buffer_put.c

buffer_read.o: buffer.h buffer_read.c compile
	./compile buffer_read.c

buffer_write.o: buffer.h buffer_write.c compile
	./compile buffer_write.c

byte.a: byte_chr.o byte_copy.o byte_cr.o byte_diff.o byte_rchr.o byte_zero.o \
	  case_diffb.o fmt_uint.o fmt_uint0.o fmt_ulong.o ip4_scan.o makelib \
	  scan_ulong.o str_chr.o str_diff.o str_len.o str_start.o \
	  uint16_pack.o uint16_unpack.o
	./makelib byte.a byte_chr.o byte_copy.o byte_cr.o byte_diff.o \
	  byte_rchr.o byte_zero.o case_diffb.o fmt_uint.o fmt_uint0.o \
	  fmt_ulong.o ip4_scan.o scan_ulong.o str_chr.o str_diff.o str_len.o \
	  str_start.o uint16_pack.o uint16_unpack.o

byte_chr.o: byte.h byte_chr.c compile
	./compile byte_chr.c

byte_copy.o: byte.h byte_copy.c compile
	./compile byte_copy.c

byte_cr.o: byte.h byte_cr.c compile
	./compile byte_cr.c

byte_diff.o: byte.h byte_diff.c compile
	./compile byte_diff.c

byte_rchr.o: byte.h byte_rchr.c compile
	./compile byte_rchr.c

byte_zero.o: byte.h byte_zero.c compile
	./compile byte_zero.c

case_diffb.o: case.h case_diffb.c compile
	./compile case_diffb.c

chkshsgr: chkshsgr.o load
	./load chkshsgr

chkshsgr.o: chkshsgr.c compile
	./compile chkshsgr.c

choose: choose.sh warn-auto.sh
	rm -f choose
	cat warn-auto.sh choose.sh >choose
	chmod 555 choose

coe.o: coe.c coe.h compile
	./compile coe.c

compile: conf-cc print-cc.sh systype warn-auto.sh
	rm -f compile
	sh print-cc.sh >compile
	chmod 555 compile

direntry.h: choose compile direntry.h1 direntry.h2 trydrent.c
	./choose c trydrent direntry.h1 direntry.h2 >direntry.h

dns.a: makelib dns_dfd.o dns_domain.o dns_dtda.o dns_ip.o dns_ipq.o dns_mx.o \
	  dns_name.o dns_nd.o dns_packet.o dns_random.o dns_rcip.o \
	  dns_rcrw.o dns_resolve.o dns_sortip.o dns_transmit.o dns_txt.o
	./makelib dns.a dns_dfd.o dns_domain.o dns_dtda.o dns_ip.o dns_ipq.o \
	  dns_mx.o dns_name.o dns_nd.o dns_packet.o dns_random.o dns_rcip.o \
	  dns_rcrw.o dns_resolve.o dns_sortip.o dns_transmit.o dns_txt.o

dns_dfd.o: compile dns_dfd.c error.h alloc.h byte.h dns.h stralloc.h \
	  gen_alloc.h iopause.h taia.h tai.h uint64.h taia.h
	./compile dns_dfd.c

dns_domain.o: compile dns_domain.c error.h alloc.h case.h byte.h dns.h \
	  stralloc.h gen_alloc.h iopause.h taia.h tai.h uint64.h taia.h
	./compile dns_domain.c

dns_dtda.o: compile dns_dtda.c stralloc.h gen_alloc.h dns.h stralloc.h \
	  iopause.h taia.h tai.h uint64.h taia.h
	./compile dns_dtda.c

dns_ip.o: compile dns_ip.c stralloc.h gen_alloc.h uint16.h byte.h dns.h \
	  stralloc.h iopause.h taia.h tai.h uint64.h taia.h
	./compile dns_ip.c

dns_ipq.o: compile dns_ipq.c stralloc.h gen_alloc.h case.h byte.h str.h \
	  dns.h stralloc.h iopause.h taia.h tai.h uint64.h taia.h
	./compile dns_ipq.c

dns_mx.o: compile dns_mx.c stralloc.h gen_alloc.h byte.h uint16.h dns.h \
	  stralloc.h iopause.h taia.h tai.h uint64.h taia.h
	./compile dns_mx.c

dns_name.o: compile dns_name.c stralloc.h gen_alloc.h uint16.h byte.h dns.h \
	  stralloc.h iopause.h taia.h tai.h uint64.h taia.h
	./compile dns_name.c

dns_nd.o: compile dns_nd.c byte.h fmt.h dns.h stralloc.h gen_alloc.h \
	  iopause.h taia.h tai.h uint64.h taia.h
	./compile dns_nd.c

dns_packet.o: compile dns_packet.c error.h dns.h stralloc.h gen_alloc.h \
	  iopause.h taia.h tai.h uint64.h taia.h
	./compile dns_packet.c

dns_random.o: compile dns_random.c dns.h stralloc.h gen_alloc.h iopause.h \
	  taia.h tai.h uint64.h taia.h taia.h uint32.h
	./compile dns_random.c

dns_rcip.o: compile dns_rcip.c taia.h tai.h uint64.h openreadclose.h \
	  stralloc.h gen_alloc.h byte.h ip4.h env.h dns.h stralloc.h \
	  iopause.h taia.h taia.h
	./compile dns_rcip.c

dns_rcrw.o: compile dns_rcrw.c taia.h tai.h uint64.h env.h byte.h str.h \
	  openreadclose.h stralloc.h gen_alloc.h dns.h stralloc.h iopause.h \
	  taia.h taia.h
	./compile dns_rcrw.c

dns_resolve.o: compile dns_resolve.c iopause.h taia.h tai.h uint64.h taia.h \
	  byte.h dns.h stralloc.h gen_alloc.h iopause.h taia.h
	./compile dns_resolve.c

dns_sortip.o: compile dns_sortip.c byte.h dns.h stralloc.h gen_alloc.h \
	  iopause.h taia.h tai.h uint64.h taia.h
	./compile dns_sortip.c

dns_transmit.o: compile dns_transmit.c socket.h uint16.h alloc.h error.h \
	  byte.h uint16.h dns.h stralloc.h gen_alloc.h iopause.h taia.h \
	  tai.h uint64.h taia.h
	./compile dns_transmit.c

dns_txt.o: compile dns_txt.c stralloc.h gen_alloc.h uint16.h byte.h dns.h \
	  stralloc.h iopause.h taia.h tai.h uint64.h taia.h
	./compile dns_txt.c

env.o: compile env.c env.h str.h
	./compile env.c

error.o: compile error.c error.h
	./compile error.c

error_str.o: compile error.h error_str.c
	./compile error_str.c

fd_copy.o: compile fd.h fd_copy.c
	./compile fd_copy.c

fd_move.o: compile fd.h fd_move.c
	./compile fd_move.c

fifo.o: compile fifo.c fifo.h hasmkffo.h
	./compile fifo.c

fmt_uint.o: compile fmt.h fmt_uint.c
	./compile fmt_uint.c

fmt_uint0.o: compile fmt.h fmt_uint0.c
	./compile fmt_uint0.c

fmt_ulong.o: compile fmt.h fmt_ulong.c
	./compile fmt_ulong.c

hasflock.h: choose compile hasflock.h1 hasflock.h2 load tryflock.c
	./choose cl tryflock hasflock.h1 hasflock.h2 >hasflock.h

hasmkffo.h: choose compile hasmkffo.h1 hasmkffo.h2 load trymkffo.c
	./choose cl trymkffo hasmkffo.h1 hasmkffo.h2 >hasmkffo.h

hassgact.h: choose compile hassgact.h1 hassgact.h2 load trysgact.c
	./choose cl trysgact hassgact.h1 hassgact.h2 >hassgact.h

hassgprm.h: choose compile hassgprm.h1 hassgprm.h2 load trysgprm.c
	./choose cl trysgprm hassgprm.h1 hassgprm.h2 >hassgprm.h

hasshsgr.h: chkshsgr choose compile hasshsgr.h1 hasshsgr.h2 load tryshsgr.c \
	  warn-shsgr
	./chkshsgr || (cat warn-shsgr; exit 1)
	./choose clr tryshsgr hasshsgr.h1 hasshsgr.h2 >hasshsgr.h

haswaitp.h: choose compile haswaitp.h1 haswaitp.h2 load trywaitp.c
	./choose cl trywaitp haswaitp.h1 haswaitp.h2 >haswaitp.h

ip4_scan.o: compile ip4_scan.c scan.h ip4.h
	./compile ip4_scan.c

iopause.h: choose compile iopause.h1 iopause.h2 load trypoll.c
	./choose clr trypoll iopause.h1 iopause.h2 >iopause.h

iopause.o: compile iopause.c iopause.h select.h tai.h taia.h uint64.h
	./compile iopause.c

load: conf-ld print-ld.sh systype warn-auto.sh
	rm -f load
	sh print-ld.sh >load
	chmod 555 load

lock_ex.o: compile hasflock.h lock.h lock_ex.c
	./compile lock_ex.c

lock_exnb.o: compile hasflock.h lock.h lock_exnb.c
	./compile lock_exnb.c

makelib: print-ar.sh systype warn-auto.sh
	rm -f makelib
	sh print-ar.sh >makelib
	chmod 555 makelib

ndelay_off.o: compile ndelay.h ndelay_off.c
	./compile ndelay_off.c

ndelay_on.o: compile ndelay.h ndelay_on.c
	./compile ndelay_on.c

open_append.o: compile open.h open_append.c
	./compile open_append.c

open_read.o: compile open.h open_read.c
	./compile open_read.c

open_trunc.o: compile open.h open_trunc.c
	./compile open_trunc.c

open_write.o: compile open.h open_write.c
	./compile open_write.c

openreadclose.o: compile error.h gen_alloc.h open.h openreadclose.c \
	  openreadclose.h readclose.h stralloc.h
	./compile openreadclose.c

pathexec_env.o: alloc.h byte.h compile env.h gen_alloc.h pathexec.h \
	  pathexec_env.c str.h stralloc.h
	./compile pathexec_env.c

pathexec_run.o: compile env.h error.h gen_alloc.h pathexec.h pathexec_run.c \
	  str.h stralloc.h
	./compile pathexec_run.c

prot.o: compile hasshsgr.h prot.c prot.h
	./compile prot.c

readclose.o: compile error.h gen_alloc.h readclose.c readclose.h stralloc.h
	./compile readclose.c

scan_ulong.o: compile scan.h scan_ulong.c
	./compile scan_ulong.c

seek_set.o: compile seek.h seek_set.c
	./compile seek_set.c

select.h: choose compile select.h1 select.h2 trysysel.c
	./choose c trysysel select.h1 select.h2 >select.h

sgetopt.o: buffer.h compile sgetopt.c sgetopt.h subgetopt.h
	./compile sgetopt.c

sig.o: compile sig.c sig.h
	./compile sig.c

sig_block.o: compile hassgprm.h sig.h sig_block.c
	./compile sig_block.c

sig_catch.o: compile hassgact.h sig.h sig_catch.c
	./compile sig_catch.c

sig_pause.o: compile hassgprm.h sig.h sig_pause.c
	./compile sig_pause.c

socket_bind.o: compile socket_bind.c byte.h socket.h uint16.h
	./compile socket_bind.c

socket_conn.o: compile socket_conn.c byte.h socket.h uint16.h
	./compile socket_conn.c

socket_tcp.o: compile socket_tcp.c ndelay.h socket.h uint16.h
	./compile socket_tcp.c

socket_udp.o: compile socket_udp.c ndelay.h socket.h uint16.h
	./compile socket_udp.c

str_chr.o: compile str.h str_chr.c
	./compile str_chr.c

str_diff.o: compile str.h str_diff.c
	./compile str_diff.c

str_len.o: compile str.h str_len.c
	./compile str_len.c

str_start.o: compile str.h str_start.c
	./compile str_start.c

stralloc_cat.o: byte.h compile gen_alloc.h stralloc.h stralloc_cat.c
	./compile stralloc_cat.c

stralloc_catb.o: byte.h compile gen_alloc.h stralloc.h stralloc_catb.c
	./compile stralloc_catb.c

stralloc_cats.o: byte.h compile gen_alloc.h str.h stralloc.h stralloc_cats.c
	./compile stralloc_cats.c

stralloc_copy.o: compile stralloc_copy.c byte.h stralloc.h gen_alloc.h
	./compile stralloc_copy.c

stralloc_eady.o: alloc.h compile gen_alloc.h gen_allocdefs.h stralloc.h \
	  stralloc_eady.c
	./compile stralloc_eady.c

stralloc_opyb.o: byte.h compile gen_alloc.h stralloc.h stralloc_opyb.c
	./compile stralloc_opyb.c

stralloc_opys.o: byte.h compile gen_alloc.h str.h stralloc.h stralloc_opys.c
	./compile stralloc_opys.c

stralloc_pend.o: alloc.h compile gen_alloc.h gen_allocdefs.h stralloc.h \
	  stralloc_pend.c
	./compile stralloc_pend.c

strerr_die.o: buffer.h compile strerr.h strerr_die.c
	./compile strerr_die.c

strerr_sys.o: compile error.h strerr.h strerr_sys.c
	./compile strerr_sys.c

subgetopt.o: compile subgetopt.c subgetopt.h
	./compile subgetopt.c

sysdeps: compile direntry.h hasflock.h hasmkffo.h hassgact.h hassgprm.h \
	  hasshsgr.h haswaitp.h iopause.h load select.h systype uint64.h \
	  uint32.h socket.lib
	rm -f sysdeps
	cat systype compile load socket.lib >>sysdeps
	grep sysdep direntry.h >>sysdeps
	grep sysdep haswaitp.h >>sysdeps
	grep sysdep hassgact.h >>sysdeps
	grep sysdep hassgprm.h >>sysdeps
	grep sysdep select.h >>sysdeps
	grep sysdep uint64.h >>sysdeps
	grep sysdep iopause.h >>sysdeps
	grep sysdep hasmkffo.h >>sysdeps
	grep sysdep hasflock.h >>sysdeps
	grep sysdep hasshsgr.h >>sysdeps
	cat sysdeps

systype: find-systype.sh trycpp.c x86cpuid.c
	sh find-systype.sh >systype

tai_now.o: compile tai.h tai_now.c uint64.h
	./compile tai_now.c

tai_pack.o: compile tai.h tai_pack.c uint64.h
	./compile tai_pack.c

tai_sub.o: compile tai.h tai_sub.c uint64.h
	./compile tai_sub.c

tai_unpack.o: compile tai.h tai_unpack.c uint64.h
	./compile tai_unpack.c

taia_add.o: compile tai.h taia.h taia_add.c uint64.h
	./compile taia_add.c

taia_approx.o: compile tai.h taia.h taia_approx.c uint64.h
	./compile taia_approx.c

taia_frac.o: compile tai.h taia.h taia_frac.c uint64.h
	./compile taia_frac.c

taia_less.o: compile tai.h taia.h taia_less.c uint64.h
	./compile taia_less.c

taia_now.o: compile tai.h taia.h taia_now.c uint64.h
	./compile taia_now.c

taia_pack.o: compile tai.h taia.h taia_pack.c uint64.h
	./compile taia_pack.c

taia_sub.o: compile tai.h taia.h taia_sub.c uint64.h
	./compile taia_sub.c

taia_uint.o: compile tai.h taia.h taia_uint.c uint64.h
	./compile taia_uint.c

time.a: iopause.o makelib tai_now.o tai_pack.o tai_sub.o tai_unpack.o \
	  taia_add.o taia_approx.o taia_frac.o taia_less.o taia_now.o \
	  taia_pack.o taia_sub.o taia_uint.o
	./makelib time.a iopause.o tai_now.o tai_pack.o tai_sub.o \
	  tai_unpack.o taia_add.o taia_approx.o taia_frac.o taia_less.o \
	  taia_now.o taia_pack.o taia_sub.o taia_uint.o

uint16_pack.o: compile uint16_pack.c uint16.h
	./compile uint16_pack.c

uint16_unpack.o: compile uint16_unpack.c uint16.h
	./compile uint16_unpack.c

uint32.h: tryulong32.c compile load uint32.h1 uint32.h2
	( (./compile tryulong32.c && ./load tryulong32 && \
	./tryulong32) >/dev/null 2>&1 \
	&& cat uint32.h2 || cat uint32.h1) >uint32.h
	rm -f tryulong32.o tryulong32

uint32_pack.o: compile uint32.h uint32_pack.c
	./compile uint32_pack.c

uint32_unpack.o: compile uint32.h uint32_unpack.c
	./compile uint32_unpack.c

uint64.h: choose compile load tryulong64.c uint64.h1 uint64.h2
	./choose clr tryulong64 uint64.h1 uint64.h2 >uint64.h

unix.a: alloc.o alloc_re.o buffer.o buffer_0.o buffer_1.o buffer_2.o \
	  buffer_get.o buffer_put.o buffer_read.o buffer_write.o coe.o \
	  env.o error.o error_str.o fd_copy.o fd_move.o fifo.o lock_ex.o \
	  lock_exnb.o makelib ndelay_off.o ndelay_on.o open_append.o \
	  open_read.o open_trunc.o open_write.o openreadclose.o \
	  pathexec_env.o pathexec_run.o prot.o readclose.o seek_set.o \
	  sgetopt.o sig.o sig_block.o sig_catch.o sig_pause.o \
	  socket_bind.o socket_conn.o socket_tcp.o socket_udp.o \
	  stralloc_cat.o stralloc_catb.o stralloc_cats.o stralloc_copy.o \
	  stralloc_eady.o stralloc_opyb.o stralloc_opys.o stralloc_pend.o \
	  strerr_die.o strerr_sys.o subgetopt.o wait_nohang.o wait_pid.o
	./makelib unix.a alloc.o alloc_re.o buffer.o buffer_0.o buffer_1.o \
	  buffer_2.o buffer_get.o buffer_put.o buffer_read.o buffer_write.o \
	  coe.o env.o error.o error_str.o fd_copy.o fd_move.o fifo.o \
	  lock_ex.o lock_exnb.o ndelay_off.o ndelay_on.o open_append.o \
	  open_read.o open_trunc.o open_write.o openreadclose.o \
	  pathexec_env.o pathexec_run.o prot.o readclose.o seek_set.o \
	  sgetopt.o sig.o sig_block.o sig_catch.o sig_pause.o \
	  socket_bind.o socket_conn.o socket_tcp.o socket_udp.o \
	  stralloc_cat.o stralloc_catb.o stralloc_cats.o stralloc_copy.o \
	  stralloc_eady.o stralloc_opyb.o stralloc_opys.o stralloc_pend.o \
	  strerr_die.o strerr_sys.o subgetopt.o wait_nohang.o wait_pid.o

wait_nohang.o: compile haswaitp.h wait_nohang.c
	./compile wait_nohang.c

wait_pid.o: compile error.h haswaitp.h wait_pid.c
	./compile wait_pid.c


# from cdb-0.75; Public domain
cdb.a: makelib cdb.o cdb_hash.o cdb_make.o uint32_pack.o uint32_unpack.o
	./makelib cdb.a cdb.o cdb_hash.o cdb_make.o uint32_pack.o \
	  uint32_unpack.o

cdb.o: compile cdb.c error.h seek.h byte.h cdb.h uint32.h
	./compile cdb.c

cdb_hash.o: compile cdb_hash.c cdb.h uint32.h
	./compile cdb_hash.c

cdb_make.o: compile cdb_make.c seek.h error.h alloc.h cdb.h uint32.h \
	  cdb_make.h buffer.h uint32.h
	./compile cdb_make.c
