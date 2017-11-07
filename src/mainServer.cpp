#include "MyChatNetwork.h"
#include <thread>

class MyChatServer : public MyChatNetwork{
public:
	MyChatServer(boost::asio::io_service& ioService_) : 
		MyChatNetwork(ioService_),
		acceptor_(ioService_, tcp::endpoint(tcp::v4(), HELLO_PORT), true)
	{
		startAccept();
	}

private:
	void startAccept(){
		waitingClient.reset(new MyChatSession(++currentClientId, ioService));
		acceptor_.async_accept(waitingClient->socket_, std::bind(&MyChatServer::onAccept, this, std::placeholders::_1));
	}

	void onAccept(const boost::system::error_code ec){
		if (ec){
			std::cout << ec.message() << std::endl;
			return;
		}
		std::cout << "Client connected, id> " << waitingClient->clientId << std::endl;

		sessions.emplace(waitingClient->clientId, waitingClient);

		int count = 0;
		for ( auto it = messageArchive.cbegin(); count < 20 && it != messageArchive.cend(); ++it, ++count)
			waitingClient->messagesToSend.push_back(*it);
		
		startWriteMessage(waitingClient);
		asynchReadHeader(waitingClient);

		startAccept();
	}


	void onMessageReceived(const std::shared_ptr<MyChatSession>& session, const std::shared_ptr<MyChatMessage>& chatMessage)override{
		if (messageArchive.size() >= 20)
			messageArchive.pop_back();

		messageArchive.push_front(chatMessage);

		for (const auto& it : sessions){
			if (it.second.get() != session.get()){
				it.second->messagesToSend.push_front(chatMessage);
				startWriteMessage(it.second);
			}
		}
	}

	void removeSession(const std::shared_ptr<MyChatSession>& session, const boost::system::error_code& ec)override{

		std::cout << "Client disconnected, id> " << session->clientId << std::endl;

		if (sessions.erase(session->clientId))
			session->socket_.close();
	}

private:
	int currentClientId = 0;
	std::shared_ptr<MyChatSession> waitingClient;
	std::map<int, std::shared_ptr<MyChatSession> > sessions;
	std::list< std::shared_ptr<MyChatMessage> > messageArchive; // push front, pop back

	tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
	std::cout << "MyChatServer" << std::endl;
	std::cout << "Type 'quit' to exit" << std::endl;

	try{
		boost::asio::io_service ioService;
		MyChatServer chatServer(ioService);

		auto ioThread = std::thread([&]{
			std::string s;
			while (std::getline(std::cin, s, '\n')){
				if (s == "quit"){
					ioService.stop();
					break;
				}
			}
		});

		ioService.run();

		ioThread.join();

	}catch(std::exception& e){
		std::cerr << e.what() << std::endl;
	}
	
	return 0;
}
