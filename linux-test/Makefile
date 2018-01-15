CXX=g++

CXXFLAGS=--std=c++17 -Wall
CXXFLAGS_Release=$(CXXFLAGS) -Ofast -march=native

SRCS=../test.cpp ../getch.cpp
#OBJS=$(subst ../,./,$(subst .cpp,.o,$(SRCS)))

all: $(SRCS) .depend alldeps
	$(CXX) $(CXXFLAGS_Release) -o test $(SRCS)

depend: .depend

.depend: $(SRCS)
	rm -f .depend
	$(CXX) $(CXXFLAGS) -MT alldeps -MM $^ >>./.depend;

clean:
	rm -f test .depend

include .depend
