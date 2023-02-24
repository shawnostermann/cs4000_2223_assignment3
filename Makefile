CPPFLAGS=-Wall -Werror -O2 -Wno-deprecated-declarations

PROGS=sharpen

all: ${PROGS}

sharpen: sharpen.cc Image.cc

clean:
	rm -f *.o ${PROGS} *.mine.*.png


test: sharpen
	@chmod +rx test.?
	-./test.1	
	-./test.2
	-./test.3
	-./test.4
	-./test.5
	-./test.6
	-./test.7
	-./test.8
