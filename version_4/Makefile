all:
	g++ -g -std=c++11 main.cpp  epoller/epoller.cpp server.cpp	\
					  threadpool/threadpool.cpp httpConn.cpp	\
					  timer/timer.cpp logs/log.cpp				\
	-o server -lpthread -lrt 

clean:
	rm -rf server
