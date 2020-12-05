
#include "thread.h"
#include "socket.h"
#include <iostream>
#include <stdlib.h>

int main(void)
{
	std::cout << "I am the client" << std::endl;

	//Open up a Socket
	Sync::Socket socket("127.0.0.1", 3340);

	// Handle any exceptions that can happen when opening a Socket.
	try
	{
		socket.Open();
	}
	catch (std::string &error)
	{

		// Cannot connect to server. most likely you didn't start the server
		// or set different ports on client/server
		std::cout << error << std::endl;

		// Exit client
		return 1;
	}
	while (true)
	{
		// Ask user for input
		std::string stringdata;

		// Get data from user
		std::cin >> stringdata;

		if (stringdata == "done")
		{
			// Return out of this.
			socket.Close();
			return 2;
		}

		// Send our user's input to the server :)
		socket.Write(Sync::ByteArray(stringdata));

		// Get the capitalized response
		Sync::ByteArray response;
		socket.Read(response);

		if (response.v.size() == 0)
		{
			std::cout << "Server did not respond correctly,"
						 " shutting down because the server is most likely dead.\n";
			socket.Close();
			return 3;
		}

		//re-use the variable from before
		//to read in response data
		stringdata = response.ToString();
		std::cout << "Server responded with: " << stringdata << std::endl;
	}
}