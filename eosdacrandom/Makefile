%.wast: %.cpp
	eosiocpp -o ${@} $^

%.abi: %.cpp
	eosiocpp -g ${@} $^

OBJS=$(patsubst %.cpp,%.wast,$(wildcard *.cpp)) $(patsubst %.cpp,%.abi,$(wildcard *.cpp))

all: $(OBJS)

clean: 
	$(RM) $(OBJS)
