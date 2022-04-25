#include "server.h"

int main()
{
	int port;
	std::cout << "输入要监听的端口：";
	std::cin >> port;
	server ser( port );
	ser.startServer();
}