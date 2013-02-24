OPENSSL_V=1.0.0e

all: openssl-$(OPENSSL_V)/out32/libeay32.lib

openssl-$(OPENSSL_V)\stamp: openssl-$(OPENSSL_V).tar.gz
	@echo Unpacking OpenSSL $(OPENSSL_V)
	7z x -y openssl-$(OPENSSL_V).tar.gz >nul:
	7z x -y openssl-$(OPENSSL_V).tar >nul:
	del openssl-$(OPENSSL_V).tar
	echo>$@

openssl-$(OPENSSL_V)/out32/libeay32.lib: openssl-$(OPENSSL_V)\stamp
	cd openssl-$(OPENSSL_V)
	perl Configure VC-WIN32
	ms\do_nasm
	nmake -f ms\nt.mak
	cd ..
