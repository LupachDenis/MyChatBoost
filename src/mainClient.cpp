#include "MyChatNetwork.h"
#include <iostream>
#include <memory>
#include <string>
#include <thread>

using boost::asio::ip::tcp;

class MyChatClient : public MyChatNetwork{
public:
	MyChatClient(boost::asio::io_service& ioService_, const std::string& hostName) 
		: MyChatNetwork(ioService_)
	{
		address = boost::asio::ip::address::from_string(hostName);

		session.reset(new MyChatSession(0, ioService));

		tryConnect(true);
	}

	void onMessageTyped(const std::string& message){
		std::vector<char> data;
		data.reserve(message.size());
		std::copy(message.cbegin(), message.cend(), std::back_inserter(data));
		std::shared_ptr<MyChatMessage> chatMessage(new MyChatMessage(0, data));
		session->messagesToSend.push_front(chatMessage);

		if (connected)
			startWriteMessage(session);
	}

private:
	void tryConnect(bool firstTime){		
		if (firstTime)
			std::cout << "Connecting to " << address.to_string() << std::endl;

		boost::asio::ip::tcp::endpoint endpoint_(address, HELLO_PORT);
		session->socket_.async_connect(endpoint_, std::bind(&MyChatClient::onConnect, this, std::placeholders::_1));
	}

	void onConnect(const boost::system::error_code& error){
		if (error){
			removeSession(session, error);
			return;
		}

		std::cout << "Connection completed" << std::endl;
		
		connected = true;

		startWriteMessage(session);
		asynchReadHeader(session);
	}
	
	void removeSession(const std::shared_ptr<MyChatSession>& session, const boost::system::error_code& ec) override{		
		
		bool wasConnected = connected;
		if (wasConnected){
			std::cout << "Disconnected" << std::endl;
		}

		session->socket_.close();
		session->inMessageHeader = { 0, 0 };
		session->outMessageHeader = { 0, 0 };
		connected = false;
		tryConnect(wasConnected);
		
	}


private:
	bool connected = false;
	boost::asio::ip::address address;
	std::shared_ptr<MyChatSession> session;

};

int main(int argc, char* argv[])
{
	std::string hostName;
	if (argc >= 2){
		hostName = argv[1];
	} else{
		hostName = "127.0.0.1";
	}

	std::cout << "MyChatClient" << std::endl;
	std::cout << "Type 'quit' to exit" << std::endl;

	try{
		boost::asio::io_service ioService;
		MyChatClient chatClient(ioService, hostName);

		auto ioThread = std::thread([&]{
			std::string s;
			while (std::getline(std::cin, s, '\n')){
				if (s == "quit"){
					ioService.stop();
					break;
				}
				ioService.post(std::bind(&MyChatClient::onMessageTyped, &chatClient, std::move(s)));
			}
		});

		ioService.run();

		ioThread.join();

	} catch (std::exception& e){
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
