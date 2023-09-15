.PHONY:all
all: dataManageTest serverTest utilTest VODServerd

dataManageTest: dataManageTest.cc
	g++ -o $@ $^ -std=c++11 -L/usr/lib64/mysql/ -ljsoncpp -lmysqlclient
utilTest: utilTest.cc
	g++ -o $@ $^ -std=c++11 -ljsoncpp
serverTest: serverTest.cc
	g++ -o $@ $^ -std=c++11 -L/usr/lib64/mysql/ -ljsoncpp -lmysqlclient -lpthread
VODServerd: VODServerd.cc
	g++ -o $@ $^ -std=c++11 -L/usr/lib64/mysql/ -ljsoncpp -lmysqlclient -lpthread

.PHONY:clean
clean:
	rm -rf dataManageTest utilTest serverTest VODServerd
