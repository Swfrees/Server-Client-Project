#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <sstream>
#include <map>
#include <fstream>

#pragma comment (lib, "ws2_32.lib")



using namespace std;

void main()
{
	//Create or clean ServerLog file
	ofstream myLog("ServerLog.txt");
	myLog.open("ServerLog.txt", std::ofstream::out | std::ofstream::trunc);
	myLog.close();

	// Winsock creation
	WSADATA wsadata;
	int wsServer = WSAStartup(WINSOCK_VERSION, &wsadata);

	// Error check
	if (wsServer != 0)
	{
		cerr << "Cannot initialize winsock server... Now ending the program" << endl;
		return;
	}

	SOCKET udpOutput = socket(AF_INET, SOCK_DGRAM, 0);

	char broadcast = '1';
	setsockopt(udpOutput, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast));

	// Create listening socket
	SOCKET listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Error check
	if (listeningSocket == INVALID_SOCKET)
	{
		cerr << "Failed to create the socket... Now ending the program" << endl;
		return;
	}

	// Get the IP address
	cout << "Starting server setup process:" << endl;
	cout << "Please input the TCP server IP address below..." << endl;
	string IP;// = "127.0.0.1";
	cin >> IP; cin.ignore();

	//cout << "Please input your IP from the ipconfig command below..." << endl;
	//string UDPIP;//personal IP
	//cin >> UDPIP; cin.ignore();

	// Get the port
	cout << "Please input the server port you plan to use below..." << endl;
	int port;// = 31549;
	cin >> port; cin.ignore();

	// Get the amount of clients
	cout << "Please input how many clients will be connecing to the server below..." << endl;
	int clientCount;
	cin >> clientCount; cin.ignore();

	// Bind the things
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	inet_pton(AF_INET, IP.c_str(), &serverAddress.sin_addr);
	int result = bind(listeningSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));

	// Error check
	if (result == SOCKET_ERROR)
	{
		cerr << "Cannot connect... bind function error... Error #" << WSAGetLastError() << endl;

		WSACleanup();
		return;
	}

	// Listening 
	result = listen(listeningSocket, 1);

	//Error check
	if (result == SOCKET_ERROR)
	{
		cerr << "Cannot connect... bind function error... Error #" << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	cout << "Server waiting for clients to connect..." << endl;

	// Master
	fd_set master;
	FD_ZERO(&master);

	FD_SET(listeningSocket, &master);

	//strings essential for rest of program

	string stringOutput;

	//Users map list
	map<int, string> registerList;
	map<int, string>::iterator iterator;

	sockaddr_in udpOutAddress;
	udpOutAddress.sin_family = AF_INET;
	udpOutAddress.sin_port = htons(31276); //different than tcp port
	udpOutAddress.sin_addr.S_un.S_addr = INADDR_BROADCAST; //personal ip from ipconfig

	while (true)
	{
		// Copy of Master
		fd_set readySet = master;
		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		string portString;
		portString = to_string(port);

		

		string BroadcastString;
		BroadcastString.append(portString);
		BroadcastString.append(",");
		BroadcastString.append(IP);
		BroadcastString.append("\n");

		char* broadcast = new char[BroadcastString.length() + 1];
		
		strcpy(broadcast, BroadcastString.c_str());

		// See who's talking to us
		int socketCount = select(0, &readySet, NULL, NULL, &timeout);

		string PortOut = "Port Number: ";
		string IPOut = " IP Address: ";

		cout << PortOut << portString << IPOut << IP << "\n";

		sendto(udpOutput, broadcast, strlen(broadcast) + 1, 0, (sockaddr*)&udpOutAddress, sizeof(udpOutAddress));

		// Error check
		if (socketCount < 0)
		{
			cerr << "Cannot connect... select function error... Error #" << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		// Loop through all the current connections / potential connect
		for (int i = 0; i < socketCount; i++)
		{
			// create socket from fd_array
			SOCKET sock = readySet.fd_array[i];

			// Is it an inbound communication?
			if (sock == listeningSocket)
			{
				// Compare Against fd_isset

				if (FD_ISSET(listeningSocket, &readySet))
				{
					// Accept a new connection
					SOCKET client = accept(listeningSocket, NULL, NULL);

					// Error check
					if (client == INVALID_SOCKET)
					{
						cerr << "Cannot connect... accept function error... Error #" << WSAGetLastError() << endl;
						WSACleanup();
						return;
					}
					//Add socket to the master set
					FD_SET(client, &master);
					ostringstream stringStream;
					stringStream << "SOCKET #" << client << ": " << "Client connected" << endl;
					string strOut = stringStream.str();
					myLog.open("ServerLog.txt", ios::app);
					myLog << strOut;
					myLog.close();
					cout << strOut;

					// Send a welcome message to the connected client
					string welcomeMsg = "Welcome to the chat:\n\nPlease type $register to register your username\n\n";
					send(client, welcomeMsg.c_str(), welcomeMsg.size() + 1, 0);
				}

			}

			else // It's an inbound message
			{
				char buf[4096];
				ZeroMemory(buf, 4096);

				// Receive message
				int bytesIn = recv(sock, buf, 4096, 0);

				if (bytesIn <= 0)
				{
					// Disconect Client

					closesocket(sock);
					ostringstream stringStream;
					stringStream << "SOCKET #" << sock << ": " << "Client disconected" << endl;
					string strOut = stringStream.str();
					myLog.open("ServerLog.txt", ios::app);
					myLog << strOut;
					myLog.close();
					cout << strOut;
					FD_CLR(sock, &master);

					//delete from users list
					iterator = registerList.find(sock);

					if (iterator != registerList.end())
					{
						registerList.erase(iterator);
					}

				}
				else
				{
					// Check to see if it's a command.
					if (buf[0] == '$')
					{
						string cmd = string(buf, bytesIn - 1);

						// $register
						if (cmd == "$register")
						{
							char buf[4096];
							ZeroMemory(buf, 4096);

							string username = "What is your username?\n";
							send(sock, username.c_str(), username.size() + 1, 0);

							// Receive message
							int bytesIn = recv(sock, buf, 4096, 0);

							registerList.insert(pair<int, string>(sock, buf));
							string regis;
							iterator = registerList.find(sock);

							if (registerList.size() <= clientCount)
							{
								ostringstream stringStream;
								stringStream << "Client " << iterator->second << ": " << "registered" << "\r\n";
								string strOut = stringStream.str();
								myLog.open("ServerLog.txt", ios::app);
								myLog << strOut;
								myLog.close();
								cout << strOut;
								//cout << PortOut << portString << IPOut << IP << "\n";
								regis = "SV_SUCCES: You are registered\n\n";
								regis += "\n\nHere is the list of commands:\n\t$getlist - Get all users connected\n\t$getlog - Get chat log\n\t$exit - Exit the client\n\n Or continue to type your message below...\n";
								send(sock, regis.c_str(), regis.size() + 1, 0);
							}

							if (registerList.size() > clientCount)
							{
								ostringstream stringStream;
								stringStream << "Client " << iterator->second << ": " << "SV_Full" << "\r\n";
								string strOut = stringStream.str();
								myLog.open("ServerLog.txt", ios::app);
								myLog << strOut;
								myLog.close();
								cout << strOut;

								regis = "SV_FULL: The chat is full...\nNow ending the client...\n";
								send(sock, regis.c_str(), regis.size() + 1, 0);

								closesocket(sock);

								iterator = registerList.find(sock);
								if (iterator != registerList.end())
								{
									registerList.erase(iterator);
								}
								FD_CLR(sock, &master);
							}

						}

						// $getlist
						if (cmd == "$getlist")
						{
							string list = "\n";
							for (iterator = registerList.begin(); iterator != registerList.end(); ++iterator)
							{
								list.append(iterator->second);
								list.append("\n");
							}
							send(sock, list.c_str(), list.size() + 1, 0);

						}

						// $getlog
						if (cmd == "$getlog")
						{
							std::ifstream showLog("ServerLog.txt");

							if (showLog.is_open())
							{
								ostringstream stringStream;
								stringStream << showLog.rdbuf() << "\n\n";
								stringOutput = stringStream.str();
								showLog.close();
								send(sock, stringOutput.c_str(), stringOutput.size() + 1, 0);
							}
						}

						// $exit
						if (cmd == "$exit")
						{
							iterator = registerList.find(sock);
							if (iterator != registerList.end())
							{
								registerList.erase(iterator);
								closesocket(sock);
							}

							//shutdown then close socket then remove from map

							FD_CLR(sock, &master);
						}

						ostringstream stringStream;

						if (iterator == registerList.end())
						{
							stringStream << "SOCKET #" << sock << ": " << cmd << "\r\n";
						}

						else
						{
							stringStream << iterator->second << ": " << cmd << "\r\n";
						}

						stringOutput = stringStream.str();
						myLog.open("ServerLog.txt", ios::app);
						myLog << stringOutput;
						myLog.close();
						cout << stringOutput;
					}

					// Send but not listening socket
					else
					{
						for (int i = 0; i < master.fd_count; i++)
						{

							SOCKET outSock = master.fd_array[i];
							iterator = registerList.find(outSock);
							ostringstream stringStream;
							if (outSock != listeningSocket && outSock != sock)
							{

								if (iterator == registerList.end())
								{
									stringStream << "SOCKET #" << outSock << ": " << buf << "\r\n";
								}
								else {
									stringStream << iterator->second << ": " << buf << "\r\n";
								}
								string strOut = stringStream.str();
								myLog.open("ServerLog.txt", ios::app);
								myLog << strOut;
								myLog.close();
								cout << strOut;
								send(outSock, strOut.c_str(), strOut.size() + 1, 0);
							}
						}
					}
				}
			}
		}
	}

	// Cleanup winsock
	WSACleanup();

	system("pause");
}