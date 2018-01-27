CXX=g++

CXXFLAGS=--std=c++17 -Wall $(USER_DEFINES)
CXXFLAGS_Release=$(CXXFLAGS) -Ofast -march=native
CXXFLAGS_Debug=$(CXXFLAGS) -ggdb -g3

SRCS=../test.cpp ../getch.cpp
OUT=./test

tests: CXXFLAGS_Actual=$(CXXFLAGS_Release)
tests: $(OUT)
	$(OUT) -u

debug: CXXFLAGS_Actual=$(CXXFLAGS_Debug)
debug: $(OUT)
	$(OUT) -u


$(OUT): $(SRCS) .depend alldeps
	$(CXX) $(CXXFLAGS_Actual) -o $(OUT) $(SRCS)

depend: .depend

.depend: $(SRCS)
	rm -f .depend
	$(CXX) $(CXXFLAGS) -MT alldeps -MM $^ >>./.depend;

clean:
	rm -f $(OUT) .depend

include .depend
