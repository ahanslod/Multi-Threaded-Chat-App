#include "thread.h"
#include "socketserver.h"
#include <stdlib.h>
#include <vector>
#include <map>
#include <string>
#include <ctime>
#include <algorithm>
#include "Semaphore.h"

using namespace std;
using namespace Sync;
int port = 3334;

// Global semaphore for variable accesses.
std::string semname = "ChatRoomSemaphore";
std::string createsem = "ChatRoomCreatorSemaphore";
Semaphore s1(createsem, 1, true);
Semaphore s2(semname, 1, true);
// Single chatroom instance represents a single chat room instance
class chatroom
{
private:
    // chatroom name
    std::string name;

    // chatroom type
    // 1 = private
    // 0 = public
    int type;

public:
    vector<std::string> messages;
    vector<std::string> usernames;

    // Constructor
    chatroom(std::string name, int type)
        : name(name), type(type)
    {
        cout << " CHAT CREATED WITH NAME: " << name << " type: " << type << endl;
    }

    // chatroom name getter function
    std::string GetType()
    {
        if (type == 1)
            return "private";
        else
            return "public";
    }
    // chatroom name getter function
    std::string GetName()
    {
        return name;
    }

    bool send_response(Socket &socket, ByteArray data)
    {
        std::string tmp = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(data.v.size()) + "\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept\r\nAccess-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type\r\n\r\n" + data.ToString();
        socket.Write(Sync::ByteArray(tmp));
    }

    // Used to send back the messages for the current chat room
    bool SendMessages(Sync::Socket &socket, size_t idx)
    {

        if (idx > messages.size() - 1)
        {
            std::string response = "errored out " + std::to_string(idx) + " " + std::to_string(messages.size());
            send_response(socket, ByteArray(response));
        }
        else
        {

            std::string response = "{ \"messages\" : [ ";
            for (int i = idx; i < messages.size(); i++)
            {
                cout << usernames.at(i) << " " << messages.at(i) << endl;
                if (i == messages.size() - 1)
                {
                    response += "{\"username\":\"" + usernames.at(i) + "\",\"message\":\"" + messages.at(i) + "\"}";
                }
                else
                {
                    response += "{\"username\":\"" + usernames.at(i) + "\",\"message\":\"" + messages.at(i) + "\"},";
                }
            }
            response += " ] }\n";
            send_response(socket, ByteArray(response));
        }
    }

    // Called when a new message comes in with %SERVERIP%/send?code=333&username=myname&message=hello sir
    bool RegisterMessage(std::string msg, std::string user)
    {
        //cout << "registering "<< msg << " "<< user<<endl;

        // CRITICAL SECTION
        // To maintain the order of messages recv
        s2.Wait();
        messages.push_back(msg);
        usernames.push_back(user);
        s2.Signal();
    }
};

class ConnectionThread : public Thread
{
    // The current connection handle
    std::map<int, chatroom> &MapOfServers;
    Sync::Socket &socket;

    // Indicates if we need to exit
    int &exit;

    std::vector<ConnectionThread *> &thread_holder;

public:
    // Constructor
    ConnectionThread(Sync::Socket &socket, int &exit, std::map<int, chatroom> &MapOfServers, std::vector<ConnectionThread *> &thread_holder)
        : socket(socket), exit(exit), MapOfServers(MapOfServers), thread_holder(thread_holder)
    {
    }

    // Destructor
    ~ConnectionThread()
    {
        try
        {
            socket.Close();
        }
        catch (...)
        {
        }
    }

    Sync::Socket &getsock()
    {
        return socket;
    }

    // Parse out the HTTP PATH out of the get req. (e.g)
    /*
        "GET /hello?hello=hi HTTP/1.1" returns "/hello?hello=hi"
    */
    std::string parse_http_path(std::string &str)
    {
        const char *space = " ";
        if (str.find("GET ") == 0)
        {
            int it = 0;

            while (it != 3)
            {
                str.erase(str.at(0));
                it++;
            }
            std::string ret;

            // Loop until the character is a space
            for (std::string::iterator itt = str.begin() + 4; itt != str.end(); itt++)
            {
                if (*itt == *space)
                    break;
                ret.push_back(*itt);
            }
            return ret;
        }
        else
            throw std::string("Not a GET request.\n");
    }

    bool send_response(ByteArray data)
    {
        std::string tmp = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(data.v.size()) + "\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept\r\nAccess-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type\r\n\r\n" + data.ToString();

        socket.Write(Sync::ByteArray(tmp));
    }
    bool send_error(ByteArray data)
    {
        std::string tmp = "HTTP/1.1 404 NOT FOUND\r\nContent-Length: " + std::to_string(data.v.size()) + "\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept\r\nAccess-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type\r\n\r\n" + data.ToString();

        socket.Write(Sync::ByteArray(tmp));
    }
    bool create(std::map<std::string, std::string> mapa)
    {

        std::string name;
        int type;

        // Generate a random code for a new chat room.
        int code = rand();

        // Check if name is passed
        if (mapa.find("name") != mapa.end())
        {
            name = mapa.at("name");
            cout << "got name: " << name;
        }
        else
        {
            std::string response = "Invalid parameter exception 1";
            send_response(ByteArray(response));
            throw response;
        }

        // Check if type is there
        if (mapa.find("type") != mapa.end())
        {
            std::string typ = mapa.at("type");
            cout << typ;
            if (typ == "private")
            {
                type = 1;
            }
            else if (typ == "public")
            {
                type = 0;
            }
            else
            {
                std::string response = "Invalid parameter exception 2";
                send_response(ByteArray(response));
                throw response;
            }
        }
        else
        {
            std::string response = "Invalid parameter exception 3";
            send_response(ByteArray(response));
            throw response;
        }

        // Create a new chat room instance
        chatroom *chat = new chatroom(name, type);

        // Send back the code to access this exact chat room
        std::string response = "{ \"code\": " + std::to_string(code) + "}\n";
        send_response(ByteArray(response));

        //CRITICAL SECTION
        // If you try to create 2 chats at the same time the index might get corrupted
        // AND this is to be safe with std::map so the map is synced when several threads
        // write to the same container, and do not cause undefined behaviour
        // Accessing maps is thread safe, but writing to them isn't
        s1.Wait();
        // insert the new server into the map
        MapOfServers.insert({code, *chat});
        s1.Signal();
    }

    //  Example: /join?code=3333
    bool join(std::map<std::string, std::string> mapa)
    {
        bool error_out = false;
        int code = 0;
        std::string response;

        // Check if code is in the response
        if (mapa.find("code") != mapa.end())
        {
            try
            {
                code = std::stoi(mapa.at("code"));
            }
            catch (...)
            {
                response = "code is not a number.\n";
                send_response(ByteArray(response));
                error_out = true;
            }
        }
        //DEBUG
        //cout << "GOT CODE "<< code << endl;
        auto it = MapOfServers.find(code);
        //std::pair<int, chatroom>

        if (it != MapOfServers.end())
        {

            // .second to get the chatroom reference
            chatroom &chat = (*it).second;

            // this assumes the chat structure is valid
            try
            {
                response = "{ \"name\": \"" + chat.GetName() + "\", \"type\": \"" + chat.GetType() + "\"}\n";
                cout << response << endl;
                send_response(ByteArray(response));
            }
            catch (std::string &e)
            {
                cout << e << endl;
                response = e;
                send_response(ByteArray(response));
            }
        }
        else
        {
            response = "this room does not exist. " + std::to_string(MapOfServers.size()) + "\n";
            send_response(ByteArray(response));
        }
    }

    bool update(std::map<std::string, std::string> mapa)
    {
        int index = 0;
        int code = 0;
        std::string response;
        bool error_out = false;
        // Get the code out of the req
        if (mapa.find("code") != mapa.end())
        {
            try
            {
                code = std::stoi(mapa.at("code"));
            }
            catch (...)
            {
                response = "code is not a number.\n";
                send_response(ByteArray(response));
                error_out = true;
            }
        }

        // This will always be 0, as the
        // logic in chatroom::SendMessages
        if (mapa.find("index") != mapa.end())
        {
            try
            {
                index = std::stoi(mapa.at("index"));
            }
            catch (...)
            {
                response = "index is not a number.\n";
                send_response(ByteArray(response));
                error_out = true;
            }
        }

        // get the access to the chatroom
        auto it = MapOfServers.find(code);
        if (it != MapOfServers.end())
        {

            chatroom &chat = (*it).second;
            // this assumes the chat structure is valid
            try
            {
                // Send messages will handle sending back all of the messages
                chat.SendMessages(socket, index);
            }
            catch (std::string &e)
            {
                cout << e << endl;
                response = e;
                send_response(ByteArray(response));
            }
        }
        else
        {
            response = "this room does not exist. " + std::to_string(MapOfServers.size()) + "\n";
            send_response(ByteArray(response));
        }
    }

    bool send(std::map<std::string, std::string> mapa)
    {
        std::string message;
        std::string user;
        int code = 0;
        std::string response;
        bool error_out = false;

        // Get the chatroom code, to which the client wants to
        // send a message to
        if (mapa.find("code") != mapa.end())
        {
            try
            {
                code = std::stoi(mapa.at("code"));
            }
            catch (...)
            {
                response = "code is not a number.\n";
                send_response(ByteArray(response));
                error_out = true;
            }
        }

        // Find the message the client wanted to send.
        if (mapa.find("msg") != mapa.end())
        {
            message = mapa.at("msg");
        }
        else
        {
            response = "no message included with the request\n";
            send_response(ByteArray(response));
            throw response;
        }

        // Get the senders' username
        if (mapa.find("user") != mapa.end())
        {
            user = mapa.at("user");
        }
        else
        {
            response = "no username included with the request\n";
            send_response(ByteArray(response));
            throw response;
        }

        if (!error_out)
        {

            // Find the correct room
            auto it = MapOfServers.find(code);
            if (it != MapOfServers.end())
            {
                chatroom &chat = (*it).second;
                // this assumes the chat structure is valid
                try
                {
                    // Registers a message to that chatroom.
                    chat.RegisterMessage(message, user);
                    response = "OK\n";
                    send_response(ByteArray(response));
                }
                catch (std::string &e)
                {
                    cout << e << endl;
                    response = e;
                    send_response(ByteArray(response));
                }
            }
            else
            {
                response = "this room does not exist. " + std::to_string(MapOfServers.size()) + "\n";
                send_response(ByteArray(response));
                throw response;
            }
        }
    }

    /*
        parse the query
        /create?name=hi&type=private
        would return a map with 2 keys: name and type
    */
    std::map<std::string, std::string> parse_http_get_path(std::string &path)
    {

        std::map<std::string, std::string> parameters;

        // Get the index of where the query starts (we already know the endpoint in handle_path)
        size_t parameter_index = path.find("?");

        // check if the index is valid, if == -1 then its not valid, if its the last
        // char then we don't need to parse the request, as there is no query after the "?"
        if (parameter_index != -1 && parameter_index != path.size() - 1)
        {
            bool end = false;

            // Iterate over characters starting from parameter_index
            for (size_t i = parameter_index + 1; i < path.size(); i++)
            {

                // ensure we don't go out of bounds, because there is no more parameters to be parsed
                if (end)
                    break;
                std::string parameter;

                // finds the index of =, to later just set the parameter name to chars between
                // the last value or "?"
                size_t value_idx = path.find("=", i);

                // check if a value got passed after the "="
                if (value_idx == -1 || value_idx == path.size() - 1)
                {
                    std::string response = "You did not pass a value to the parameter.\n";
                    send_response(ByteArray(response));
                    throw std::string("Could not parse the http request.\n");
                }

                // parameter is crafted from in between last val/"?" and "="
                parameter = path.substr(i, value_idx - i);

                // parse finds the location of the next parameter, if it's -1 then we need to exit right after saving
                // the current parameter to the parameter map
                size_t valend = path.find("&", i);
                if (valend == -1)
                {
                    valend = path.size();
                    end = true;
                }
                // DEBUG
                //cout << "got parameter: " << parameter<< " " <<value_idx+1 << " "<< valend-value_idx-1 <<endl;

                // Add the current parameter to the map
                std::string value = path.substr(value_idx + 1, valend - value_idx - 1);
                parameters.insert({parameter, value});

                // Set the iterator to the beginning of next parameter:value pair
                i = valend;
            }
        }
        else
        {
            std::string response = "no arguments provided.\n";
            send_response(ByteArray(response));
            throw std::string("Could not parse the http request.\n");
        }
        return parameters;
    }

    bool handle_path(std::string &path)
    {
        if (path.find("welcome") == 1)
        {
            std::string response = "Welcome! Please enter your username to use this chat app.\n";

            send_response(ByteArray(response));
        }
        else if (path.find("create") == 1)
        {
            try
            {
                auto mapa = parse_http_get_path(path);
                create(mapa);
            }
            catch (std::string &err)
            {
                cout << err << endl;
            }
        }
        else if (path.find("join") == 1)
        {
            try
            {
                auto mapa = parse_http_get_path(path);
                join(mapa);
            }
            catch (std::string &err)
            {
                cout << err << endl;
            }
        }
        else if (path.find("update") == 1)
        {
            try
            {
                auto mapa = parse_http_get_path(path);
                update(mapa);
            }
            catch (std::string &err)
            {
                cout << err << endl;
            }
        }
        else if (path.find("send") == 1)
        {
            try
            {
                auto mapa = parse_http_get_path(path);
                send(mapa);
            }
            catch (std::string &err)
            {
                cout << err << endl;
            }
        }
        else
        {
            send_error(ByteArray("invalid data"));
        }
    }

    virtual long ThreadMain()
    {
        Sync::ByteArray conn_data;
        while (exit == 0)
        {
            try
            {
                //Wait for data from this connection
                int socketResult = socket.Read(conn_data);

                // The client closed the connection with us, clean this thread up!
                if (socketResult == 0)
                {

                    // Erase that thread from the thread_holder vector, as we are about to exit
                    // and close the connection on our side
                    auto it = std::find(thread_holder.begin(), thread_holder.end(), this);
                    if (it != thread_holder.end())
                    {
                        thread_holder.erase(it);
                    }
                    // Close this socket on our side.
                    socket.Close();

                    // Exit out of this loop, which should make us exit out of the whole thread too.
                    break;
                }

                // Currently, requests get fragmented into 256 byte chunks and to make the server
                // parse the whole request we would need to add coming in requests live, until
                // the client closes the socket (socketResult = 0), but because we have a keepalive
                // HTTP session, to prevent problems with packet fragmentation, the req size is
                // capped at 256.
                else if (socketResult == 256)
                {
                    std::string string_data = conn_data.ToString();
                    std::string path = parse_http_path(string_data);
                    handle_path(path);
                }
            }
            // Handle any exceptions
            catch (std::string err)
            {
                std::cout << err << std::endl;
            }
        }
    }
};

// This thread handles the server operations
class ServerThread : public Thread
{
private:
    // Handle to the server
    Sync::SocketServer &server;
    std::vector<ConnectionThread *> thread_holder;

    int &exit;

public:
    ServerThread(Sync::SocketServer &server, int &exit)
        : server(server), exit(exit)
    {
    }

    ~ServerThread()
    {
        for (auto t : thread_holder)
        {
            try
            {
                t->getsock().Close();
            }
            catch (...)
            { // ignore if it fails
            }
        }
        exit = 1;
    }

    virtual long ThreadMain()
    {
        int exit = 0;
        std::map<int, chatroom> MapOfServers;
        int conn = 0;
        // Create a single public room
        chatroom *chat = new chatroom("Main Lobby", 0);

        // insert the new server into the map
        MapOfServers.insert({0, *chat});

        while (exit == 0)
        {
            conn += 1;
            std::cout << "Connection: " << conn << std::endl;
            // Wait for a new connection from the client
            Sync::Socket *newConnection = new Sync::Socket(server.Accept());

            // Process the client request
            thread_holder.push_back(
                new ConnectionThread(*newConnection, exit, MapOfServers, thread_holder));
        }
    }
};

int main(void)
{
    // Init random number generator to gen the server codes
    srand(time(NULL));

    std::cout << "I am the server. Enter kills the server." << std::endl;
    int exit = 0;
    // Start the server and pass the control to another thread
    try
    {
        // Open socket
        Sync::SocketServer serv(port);

        // Start listening
        ServerThread serverthread(serv, exit);
        // Wait for input to shut down the server
        Sync::FlexWait cinWaiter(1, stdin);
        cinWaiter.Wait();
        cin.get();
        exit = 1;

        // Shutdown and cleanup
        serv.Shutdown();
    }
    catch (std::string e)
    {
        cout << e << endl;
    }
}
