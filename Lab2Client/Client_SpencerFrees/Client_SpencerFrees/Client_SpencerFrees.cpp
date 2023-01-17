#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")



using namespace std;

void main()
{

	// IP Address of the server
	string ipAddress = "127.0.0.1";
	//cout << "What IP you want to connect to?" << endl;
	//cin >> ipAddress; cin.ignore();

	// Listening port # on the server
	int port = 31549;
	//cout << "What Port you want to use?" << endl;
	//cin >> port; cin.ignore();

	// Initialize WinSock
	WSADATA wsadata;
	int wsClient = WSAStartup(WINSOCK_VERSION, &wsadata);

	// Error check
	if (wsClient != 0)
	{
		cerr << "Cannot start Winsock... Error #" << wsClient << endl;
		return;
	}

	// Create socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Error check
	if (sock == INVALID_SOCKET)
	{
		cerr << "Cannot create socket... Error #" << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	SOCKET udpInput;
	sockaddr_in udpInputAddress;

	udpInputAddress.sin_family = AF_INET;
	udpInputAddress.sin_port = htons(31276);//same port as server
	udpInputAddress.sin_addr.S_un.S_addr = INADDR_ANY; //ip from ipconfig

	//start udp
	udpInput = socket(AF_INET, SOCK_DGRAM, 0);

	if (udpInput == SOCKET_ERROR)
	{
		cerr << "Socket creation was not successful... Error #" << WSAGetLastError() << endl;
		WSACleanup();
	}

	char Recieve = '1';
	setsockopt(udpInput, SOL_SOCKET, SO_REUSEADDR, (char*)&Recieve, sizeof(Recieve));
	int bindresult = bind(udpInput, (SOCKADDR*)&udpInputAddress, sizeof(udpInputAddress));

	if (bindresult == SOCKET_ERROR)
	{
		cerr << "Cannot connect... bind function error... Error #" << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	char broadcast[127];
	int stringLength = sizeof(struct sockaddr_in);

	recvfrom(udpInput, broadcast, 127, 0, (sockaddr*)&udpInputAddress, &stringLength);

	string RecievedData = (string)broadcast;

	closesocket(udpInput);
	shutdown(udpInput, SD_RECEIVE);

	string RevievedIP;
	string RecievedPort;

	bool isAddr = false;
	// parse the connection data out of the string;
	for (int i = 0; i < RecievedData.size(); i++) {
		if (RecievedData.at(i) == ',' || RecievedData.at(i) == '\n')
		{
			isAddr = true;
		}
		else {
			if (isAddr)
			{
				RevievedIP += RecievedData.at(i);
			}
			else
			{
				RecievedPort += RecievedData.at(i);
			}
		}
	}

	string PortOut = "Port Number: ";
	string IPOut = " IP Address: ";

	cout << PortOut << RecievedPort << IPOut << RevievedIP << "\n";

	//client broadcast information to print out here

	// end udp

	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(atoi(RecievedPort.c_str()));// atoi((char*)RecievedPort.c_str()); //htons(54000);
	//udpInputAddress.sin_addr.S_un.S_addr = inet_addr((char*)RevievedIP.c_str());
	inet_pton(AF_INET, RevievedIP.c_str(), &hint.sin_addr);

	// Connect
	int connectResult = connect(sock, (sockaddr*)&hint, sizeof(hint));

	// Error check
	if (connectResult == SOCKET_ERROR)
	{
		cerr << "Cannot connect to server... Error #" << WSAGetLastError() << endl;
		closesocket(sock);
		WSACleanup();
		return;
	}



	// Do-while loop to send and receive data
	char buf[4096];
	string userInput;

	ZeroMemory(buf, 4096);
	int bytesReceived = recv(sock, buf, 4096, 0);

	ofstream myLog("ClientLog.txt");
	myLog.open("ClientLog.txt", std::ofstream::out | std::ofstream::trunc);
	myLog.close();

	if (bytesReceived > 0)
	{
		// Echo response to console
		cout << "SERVER> " << string(buf, 0, bytesReceived) << endl;
	}

	bool BroadcastInfo = false;

	do
	{
		cout << "> ";
		getline(cin, userInput);

		// Checking user input
		if (userInput.size() > 0)
		{
			// Send the message
			int sendResult = send(sock, userInput.c_str(), userInput.size() + 1, 0);

			if (sendResult != SOCKET_ERROR)
			{
				// Wait for response
				ZeroMemory(buf, 4096);
				int bytesReceived = recv(sock, buf, 4096, 0);

				if (bytesReceived > 0)
				{
					// Echo response to console
					cout << "SERVER> " << string(buf, 0, bytesReceived) << endl;
					myLog.open("ClientLog.txt", ios::app);
					myLog << string(buf, 0, bytesReceived);
					myLog.close();
				}

				// Error check
				else
				{
					cerr << "Cannot recieve from the server... Error #" << WSAGetLastError() << endl;
					closesocket(sock);
					WSACleanup();
					system("pause");
					return;
				}
			}

			else
			{
				cerr << "Cannot send to the server... Error #" << WSAGetLastError() << endl;
				closesocket(sock);
				WSACleanup();
				system("pause");
				return;
			}
		}
	} while (userInput.size() > 0);

	// Gracefully close down everything
	closesocket(sock);
	WSACleanup();
}