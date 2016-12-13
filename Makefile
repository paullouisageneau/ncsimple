CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-O3 -c -Wall
LDFLAGS=-O3
LDLIBS=

SRCS=$(shell printf "%s " *.cpp)
OBJS=$(subst .cpp,.o,$(SRCS))

all: ncredundancy
	
%.o: %.cpp
	$(CXX) $(CPPFLAGS) -I. -MMD -MP -o $@ -c $<
	
-include $(subst .o,.d,$(OBJS))
	
ncredundancy: $(OBJS)
	$(CXX) $(LDFLAGS) -o ncsimple $(OBJS) $(LDLIBS) 
	
clean:
	$(RM) *.o *.d

dist-clean: clean
	$(RM) ncsimple
	$(RM) *~

