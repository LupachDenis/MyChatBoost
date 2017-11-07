#ifndef MYCHATMESSAGE_H
#define MYCHATMESSAGE_H

#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <iostream>
#include <boost/cstdint.hpp>
#include <memory>
#include <vector>
#include <list>

#define HELLO_PORT 22001

#pragma pack(push, 1)
struct MyChatMessageHeader{
	uint16_t messageLength;
	int32_t clientId;
};
#pragma pack(pop)

struct MyChatMessage : public std::enable_shared_from_this<MyChatMessage>{
	int clientId;
	std::vector<char> messageData;

	MyChatMessage(int clientId_, const std::vector<char>& messageData_) : clientId(clientId_), messageData(messageData_){}
};

using boost::asio::ip::tcp;

struct MyChatSession : public std::enable_shared_from_this<MyChatSession>, boost::noncopyable{
	int clientId;

	tcp::socket socket_;

	MyChatMessageHeader inMessageHeader;
	std::vector<char> inMessageData;

	MyChatMessageHeader outMessageHeader;
	std::shared_ptr<MyChatMessage> outMessage;

	std::list<std::shared_ptr<MyChatMessage> > messagesToSend; // push front, pop back

	MyChatSession(int clientId_, boost::asio::io_service& ioService) : clientId(clientId_), socket_(ioService){
		inMessageHeader = { 0, 0 };
		outMessageHeader = { 0, 0 };
	}
};

class MyChatNetwork : boost::noncopyable{
public:
	MyChatNetwork(boost::asio::io_service& ioService_) : ioService(ioService_){

	}
protected:
	void asynchReadHeader(const std::shared_ptr<MyChatSession>& session){

		boost::asio::async_read(session->socket_, boost::asio::buffer(&session->inMessageHeader, sizeof(MyChatMessageHeader)),
			std::bind(&MyChatNetwork::onReadHeader, this, session, std::placeholders::_1, std::placeholders::_2));
	}

	void onReadHeader(const std::shared_ptr<MyChatSession>& session, const boost::system::error_code& ec, std::size_t bytes){

		if (ec){
			removeSession(session, ec);
			return;
		}

		boost::endian::little_to_native_inplace(session->inMessageHeader.messageLength);
		boost::endian::little_to_native_inplace(session->inMessageHeader.clientId);
		session->inMessageData.resize(session->inMessageHeader.messageLength);

		boost::asio::async_read(session->socket_, boost::asio::buffer(session->inMessageData.data(), session->inMessageHeader.messageLength),
			std::bind(&MyChatNetwork::onReadMessage, this, session, std::placeholders::_1, std::placeholders::_2));
	}

	void onReadMessage(const std::shared_ptr<MyChatSession>& session, const boost::system::error_code& ec, std::size_t bytes){

		if (ec){
			removeSession(session, ec);
			return;
		}

		int clientId = session->clientId ? session->clientId : session->inMessageHeader.clientId;

		std::cout << clientId << "> ";
		std::copy(session->inMessageData.cbegin(), session->inMessageData.cend(), std::ostream_iterator<char>(std::cout));
		std::cout << std::endl;

		std::shared_ptr<MyChatMessage> chatMessage = std::make_shared<MyChatMessage>(clientId, std::move(session->inMessageData));
		onMessageReceived(session, chatMessage);

		asynchReadHeader(session);
	}

	void startWriteMessage(const std::shared_ptr<MyChatSession>& session){

		if (session->outMessageHeader.clientId || session->outMessageHeader.messageLength)
			return;

		if (!session->messagesToSend.size())
			return;

		session->outMessage = session->messagesToSend.back();
		session->messagesToSend.pop_back();
		MyChatMessage* message_ = session->outMessage.get();

		session->outMessageHeader.messageLength = boost::endian::native_to_little<uint16_t>(message_->messageData.size());
		session->outMessageHeader.clientId = boost::endian::native_to_little<int32_t>(message_->clientId);

		boost::asio::async_write(session->socket_,
			boost::asio::buffer(&session->outMessageHeader, sizeof(MyChatMessageHeader)),
			std::bind(&MyChatNetwork::onWriteHeader, this, session, std::placeholders::_1, std::placeholders::_2)
			);
	}

	void onWriteHeader(const std::shared_ptr<MyChatSession>& session, const boost::system::error_code& ec, std::size_t bytes){

		if (ec){
			removeSession(session, ec);
			return;
		}

		boost::asio::async_write(session->socket_,
			boost::asio::buffer(session->outMessage.get()->messageData.data(), session->outMessageHeader.messageLength),
			std::bind(&MyChatNetwork::onWriteMessage, this, session, std::placeholders::_1, std::placeholders::_2)
			);
	}

	void onWriteMessage(const std::shared_ptr<MyChatSession>& session, const boost::system::error_code& ec, std::size_t bytes){

		if (ec){
			removeSession(session, ec);
			return;
		}

		session->outMessageHeader.messageLength = 0;
		session->outMessageHeader.clientId = 0;

		startWriteMessage(session);
	}


	virtual void onMessageReceived(const std::shared_ptr<MyChatSession>& session, const std::shared_ptr<MyChatMessage>& chatMessage){};
	virtual void removeSession(const std::shared_ptr<MyChatSession>& session, const boost::system::error_code& ec) = 0;

protected:
	boost::asio::io_service& ioService;
};


#endif
