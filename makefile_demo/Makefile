# g++ -g -o testclient.out testclient.cpp ../stringutil.cpp -I../ -std=c++0x
# g++ -g -o testTimeout.out testTimeout.cpp ../stringutil.cpp -I../ -std=c++0x
# g++ -g -o testPartial.out testPartial.cpp ../stringutil.cpp -I../ -std=c++0x

CXXFLAGS = -g -I../ -std=c++0x
OBJECTS = testFalse.out testCorrect.out testKeepalive.out testPartial.out testReqout.out testRcvout.out testBigfile.out

.PHONY : all
all : $(OBJECTS)

$(OBJECTS) : %.out:%.cpp 
	$(CXX) $(CXXFLAGS) -o $@ $< ../stringutil.cpp

.PHONY : clean
clean :
	rm -f *.out

