build-all:build
PACKET=/home/chlaws/packet
APR_INC=$(PACKET)/apr-1.4.6/include
APR_LIB=$(PACKET)/apr-1.4.6/.libs/libapr-1.a
ZLIB_INC=$(PACKET)/zlib-1.2.6
ZLIB_LIB=$(PACKET)/zlib-1.2.6/libz.a

PSL_INC=$(PACKET)/polarssl-1.1.3/include
PSL_LIB=$(PACKET)/polarssl-1.1.3/library/libpolarssl.a

BOOST_INC=$(PACKET)/boost_1_47_0
BOOST_LIB=$(BOOST_INC)/stage/lib/libboost_thread.a
LIB=$(APR_LIB) $(ZLIB_LIB) $(PSL_LIB) $(BOOST_LIB) -lpthread
INC=-I. -I$(APR_INC) -I$(ZLIB_INC) -I$(PSL_INC) -I$(BOOST_INC)
CFLAG=-g -DAPR  -DUSE_BOOST_THREAD $(INC)
CXX=g++

SOURCE=$(wildcard *.cpp)
OBJ=$(patsubst %.cpp,%.o,$(SOURCE))
OBJS=$(filter-out main.o,$(OBJ))
.cpp.o:$(SOURCE)
	$(CXX) $(CFLAG) -c $< -o $@

build:$(OBJS)
	$(CXX) $(CFLAG) $(OBJS) main.cpp -o guahao $(LIB)
clean:
	rm -rf *.o
test-all:test-response test-request test-parsehtml

test-response:
	g++ -g -DDEBUG_PARSERESPONSE ParseResponse.cpp -o $@

test-request:
	g++ -g -DDEBUG_REQUESTHEADER RequestHeader.cpp -o $@

test-parsehtml:ParseHtml.cpp
	g++ -g -DDEBUG_PARSEHTML     ParseHtml.cpp -o $@


.PHONY:build
