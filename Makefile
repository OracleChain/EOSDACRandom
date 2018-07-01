CXXFLAGS=`pkg-config --cflags libcrypto++` -O2
LIBS=`pkg-config --libs libcrypto++` -lboost_system -lboost_filesystem -lboost_thread -lpthread
OBJS=$(patsubst %.cpp,%.o,$(wildcard *.cpp))

all: randseedserver contract

randseedserver: randseedserver.o
	$(CXX) $^ $(LIBS) -o ${@}

.PHONY: contract install
contract:
	make -C eosdacrandom
	make -C requester

install: all
	bash set_contract.sh

test:
	./randseedserver &
	bash looprequest.sh

clean:
	$(RM) $(OBJS) randseedserver
	make -C eosdacrandom clean
	make -C requester clean
