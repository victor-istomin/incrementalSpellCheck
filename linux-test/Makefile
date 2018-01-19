CXX=g++

CXXFLAGS=--std=c++17 -Wall $(USER_DEFINES)
CXXFLAGS_Release=$(CXXFLAGS) -Ofast -march=native

SRCS=../test.cpp ../getch.cpp
OUT=./test

tests: $(OUT)
	$(OUT) -u

$(OUT): $(SRCS) .depend alldeps
	$(CXX) $(CXXFLAGS_Release) -o $(OUT) $(SRCS)

depend: .depend

.depend: $(SRCS)
	rm -f .depend
	$(CXX) $(CXXFLAGS) -MT alldeps -MM $^ >>./.depend;

clean:
	rm -f $(OUT) .depend

include .depend
