CXX=g++

CXXFLAGS=--std=c++17 -Wall $(USER_DEFINES)
CXXFLAGS_Release=$(CXXFLAGS) -Ofast -march=native

SRCS=../test.cpp ../getch.cpp

all: $(SRCS) .depend alldeps
	$(CXX) $(CXXFLAGS_Release) -o test $(SRCS)

depend: .depend

.depend: $(SRCS)
	rm -f .depend
	$(CXX) $(CXXFLAGS) -MT alldeps -MM $^ >>./.depend;

clean:
	rm -f test .depend

include .depend
