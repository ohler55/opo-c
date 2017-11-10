
all:
	make -C src

clean:
	make -C src clean
	make -C test clean
	rm -rf include lib

test: all
	make -C test test
