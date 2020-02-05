IDIR =../include
#MCF_DIR = ./ModifiedCuckooFilter/src/
CC=g++

CFLAGS=-I$(IDIR) -pg -g -Ofast -std=c++11
# OPT = -Ofast -pg -g

# CFLAGS += --std=c++11 -fno-strict-aliasing -Wall -c -I. -I$(IDIR) -I$(MCF_DIR) -I./include -I/usr/include/ -I/home/paris/.linuxbrew/Cellar/openssl/1.0.2s/include -I./src/ $(OPT)

# CFLAGS += --std=c++11 -fno-strict-aliasing -Wall -c -I. -I$(IDIR) -I./include -I/usr/include/ -I/home/paris/.linuxbrew/Cellar/openssl/1.0.2s/include -I./src/ $(OPT)


LDFLAGS+= -Wall -lpthread -L /home/paris/.linuxbrew/Cellar/openssl/1.0.2s/lib/ -lssl -lcrypto

#LIBOBJECTS = \
#	./ModifiedCuckooFilter/src/hashutil.o \

#HEADERS = $(wildcard ./ModifiedCuckooFilter/src/*.h)
# ALIB = libcuckoofilter.a

ODIR=obj
LDIR =../lib

LIBS=-lm

_DEPS = CRIoT.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_TEST_OBJ = test.o ClientSock.o common.o farmhash.o input_types.o
TEST_OBJ = $(patsubst %,$(ODIR)/%,$(_TEST_OBJ))

_TEST_OTHELLO_OBJ = othello_test.o ClientSock.o common.o farmhash.o input_types.o
TEST_OTHELLO_OBJ = $(patsubst %,$(ODIR)/%,$(_TEST_OTHELLO_OBJ))

_TEST_BALANCE_OTHELLO_OBJ = balance_othello_test.o ClientSock.o common.o farmhash.o input_types.o
TEST_BALANCE_OTHELLO_OBJ = $(patsubst %,$(ODIR)/%,$(_TEST_BALANCE_OTHELLO_OBJ))

_TEST_FILTER_CASCADES_OBJ = filter_cascades_test.o ClientSock.o common.o farmhash.o input_types.o
TEST_FILTER_CASCADES_OBJ = $(patsubst %,$(ODIR)/%,$(_TEST_FILTER_CASCADES_OBJ))

_EVAL_ALL_OBJ = eval_all.o ClientSock.o common.o farmhash.o input_types.o
TEST_EVAL_ALL_OBJ = $(patsubst %,$(ODIR)/%,$(_EVAL_ALL_OBJ))

_MO_OBJ = mo.o ClientSock.o common.o farmhash.o input_types.o
MO_OBJ = $(patsubst %,$(ODIR)/%,$(_MO_OBJ))

_VO_MEM_OBJ = vo_mem_vs_ratio.o ClientSock.o common.o farmhash.o input_types.o
VO_MEM_OBJ = $(patsubst %,$(ODIR)/%,$(_VO_MEM_OBJ))

_MU_TEST_OBJ = mulit_test.o ClientSock.o common.o farmhash.o input_types.o
MU_TEST_OBJ = $(patsubst %,$(ODIR)/%,$(_MU_TEST_OBJ))

_MEM_TEST_OBJ = memory_eval.o ClientSock.o common.o farmhash.o input_types.o
MEM_TEST_OBJ = $(patsubst %,$(ODIR)/%,$(_MEM_TEST_OBJ))

_MEM_VS_R_OBJ = mem_vs_ratio.o ClientSock.o common.o farmhash.o input_types.o
MEM_VS_R_OBJ = $(patsubst %,$(ODIR)/%,$(_MEM_VS_R_OBJ))

_CRIOT_TEST = CRIoT_test.o ClientSock.o common.o farmhash.o input_types.o
CRIOT_TEST = $(patsubst %,$(ODIR)/%,$(_CRIOT_TEST))

_INSERT_THRPUT = insert_thrput.o ClientSock.o common.o farmhash.o input_types.o
INSERT_THRPUT = $(patsubst %,$(ODIR)/%,$(_INSERT_THRPUT))

_FLIP_THRPUT = flip_thrput.o ClientSock.o common.o farmhash.o input_types.o
FLIP_THRPUT = $(patsubst %,$(ODIR)/%,$(_FLIP_THRPUT))

_LONG_TERM_INSERT = long_term_insert.o ClientSock.o common.o farmhash.o input_types.o
LONG_TERM_INSERT = $(patsubst %,$(ODIR)/%,$(_LONG_TERM_INSERT))

_LONG_TERM_VALUEFLIP = long_term_valueFlip.o ClientSock.o common.o farmhash.o input_types.o
LONG_TERM_VALUEFLIP = $(patsubst %,$(ODIR)/%,$(_LONG_TERM_VALUEFLIP))

_LONG_TERM_INSERT_CONFIG = long_term_insert_config.o ClientSock.o common.o farmhash.o input_types.o
LONG_TERM_INSERT_CONFIG = $(patsubst %,$(ODIR)/%,$(_LONG_TERM_INSERT_CONFIG))

_LONG_TERM_INSERT_RATIO = long_term_insert_ratio.o ClientSock.o common.o farmhash.o input_types.o
LONG_TERM_INSERT_RATIO = $(patsubst %,$(ODIR)/%,$(_LONG_TERM_INSERT_RATIO))

_QUERY_THRPUT = query_thrput.o ClientSock.o common.o farmhash.o input_types.o
QUERY_THRPUT = $(patsubst %,$(ODIR)/%,$(_QUERY_THRPUT))

_CRIOT_SERVER = CRIoT_Server.o ClientSock.o common.o farmhash.o input_types.o ClientSocket.o ServerSocket.o Socket.o 
CRIOT_SERVER = $(patsubst %,$(ODIR)/%,$(_CRIOT_SERVER))

_CRIOT_CLIENT = CRIoT_Client.o ClientSock.o common.o farmhash.o input_types.o ClientSocket.o ServerSocket.o Socket.o 
CRIOT_CLIENT = $(patsubst %,$(ODIR)/%,$(_CRIOT_CLIENT))

_SERVER_OBJ = server.o ClientSock.o common.o farmhash.o input_types.o ClientSocket.o ServerSocket.o Socket.o TinyCRServer.o
SERVER_OBJ = $(patsubst %,$(ODIR)/%,$(_SERVER_OBJ))


$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server.o: server.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)


#$(ODIR)/%.o: %.cc ${HEADERS} Makefile
#	$(CC) $(CFLAGS) $< -o $@

#$(ALIB): $(LIBOBJECTS)
#	$(AR) rcs $@ $(LIBOBJECTS)

all: othello_test balance_othello_test filter_cascades_test eval_all mo


CRIoT_Server: $(CRIOT_SERVER)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

CRIoT_Client: $(CRIOT_CLIENT)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

query_thrput: $(QUERY_THRPUT)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

long_term_insert_ratio: $(LONG_TERM_INSERT_RATIO)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

long_term_insert_config: $(LONG_TERM_INSERT_CONFIG)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
	
long_term_valueFlip: $(LONG_TERM_VALUEFLIP)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

long_term_insert: $(LONG_TERM_INSERT)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
	
flip_thrput: $(FLIP_THRPUT)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

insert_thrput: $(INSERT_THRPUT)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

criot_test: $(CRIOT_TEST) 
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

mem_vs_ratio: $(MEM_VS_R_OBJ) 
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

memory_eval: $(MEM_TEST_OBJ) 
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

mulit_test: $(MU_TEST_OBJ) 
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

vo_mem_vs_ratio: $(VO_MEM_OBJ) 
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

mo: $(MO_OBJ) 
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

test: $(TEST_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

othello_test: $(TEST_OTHELLO_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)


balance_othello_test: $(TEST_BALANCE_OTHELLO_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

filter_cascades_test: $(TEST_FILTER_CASCADES_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

eval_all: $(TEST_EVAL_ALL_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server: $(SERVER_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
