
#include "../includes/ServerManager.hpp"

ServerManager::ServerManager(std::vector<ServerConfig> servers) {
	for (size_t i = 0; i < servers.size(); i++) {
		std::vector<int> ports = servers[i].getPorts();
		for (size_t j = 0; j < ports.size(); j++)
			this->_servers.push_back(Server(servers[i], ports[j]));
	}
}

ServerManager::~ServerManager() {
	for (size_t i = 0; i < this->_servers.size(); i++)
        close(this->_servers[i].getServerSocket());
}

void ServerManager::createSockets() {
	std::map<std::string, int> checkIpPort;
	std::string ipPort;
	for (size_t i = 0; i < this->_servers.size(); i++) {
		int opt = 1;
		ipPort = this->_servers[i].getHost() + ":" + itostr(this->_servers[i].getPort());
		
		for (std::map<std::string, int>::iterator it = checkIpPort.begin(); it != checkIpPort.end(); it++) {
			if (ipPort == it->first) {
				this->_servers[i].setServerSocket(it->second);
				this->_servers[i].setIsDup(true);
				break;
			}
		}
		if (this->_servers[i].getServerSocket() != -1)
			continue;
		this->_servers[i].setServerSocket(socket(AF_INET, SOCK_STREAM, 0));
		if (this->_servers[i].getServerSocket() == -1)
			continue;
		if (setsockopt(this->_servers[i].getServerSocket(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
			throw SocketFailed();
		if (bind(this->_servers[i].getServerSocket(), (struct sockaddr *)&this->_servers[i].getAddress(), this->_servers[i].getAddrLen()) < 0)
			throw SocketFailed();
		if (listen(this->_servers[i].getServerSocket(), 10) < 0)
			throw SocketFailed();
		checkIpPort.insert(std::make_pair(ipPort, this->_servers[i].getServerSocket()));
	}
}

void ServerManager::controlSockets(int epollFd) {
	struct epoll_event event;
	event.events = EPOLLIN | EPOLLHUP;
	for (size_t i = 0; i < this->_servers.size(); i++) {
		if (this->_servers[i].getIsDup() == true)
			continue;
		event.data.fd = this->_servers[i].getServerSocket();
		if (epoll_ctl(epollFd, EPOLL_CTL_ADD, this->_servers[i].getServerSocket(), &event) == -1)
			throw SocketFailed();
	}
}

int ServerManager::compareServerSocket(int eventFd) {
	for (size_t i = 0; i < this->_servers.size(); i++)
        if (eventFd == this->_servers[i].getServerSocket())
            return i;
    return -1;
}

void ServerManager::handleServerSocket(int epollFd, int index) {
	struct epoll_event event;
	int clientSocket = accept(this->_servers[index].getServerSocket(), (struct sockaddr *)&this->_servers[index].getAddress(), (socklen_t *)&this->_servers[index].getAddrLen());
	if (clientSocket < 0)
		throw SocketFailed();
	event.events = EPOLLIN;
	event.data.fd = clientSocket;
	this->_servers[index].addClientSocket(clientSocket);
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1)
		throw SocketFailed();
}

int ServerManager::compareClientSocket(int eventFd, bool forClose) {
	for (size_t i = 0; i < this->_servers.size(); i++) {
		std::vector<int> clientSockets = this->_servers[i].getClientSockets();
		for (size_t j = 0; j < clientSockets.size(); j++) {
			if (eventFd == clientSockets[j]) {
				if (forClose) {
					this->_servers[i].rmClientSocket(j);
					close(eventFd);
					return -1;
				}
				return this->_servers[i].getServerSocket();
			}
		}
	}
    return -1;
}

std::string toHex(const std::string& input) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < input.size(); ++i) {
        ss << std::setw(2) << static_cast<int>(static_cast<unsigned char>(input[i])) << " ";
    }
    return ss.str();
}


void ServerManager::handleClientSocket(epoll_event event)
{
    char buffer[BUF_SIZE] = {0};
    int serverSocket = compareClientSocket(event.data.fd, 0);
    if (serverSocket == -1) {
        std::cerr << "Socket not found or already closed: " << event.data.fd << std::endl;
        return;
    }
    if (event.events & EPOLLHUP) {
        std::cout << "EPOLLHUP event, closing socket: " << event.data.fd << std::endl;
        compareClientSocket(event.data.fd, true);
        return;
    } 
    else if (event.events & EPOLLIN) {
        std::string requestData;
        int bytesRead;

        // Reading the initial part of the request
        while ((bytesRead = recv(event.data.fd, buffer, BUF_SIZE, 0)) > 0) {
            requestData.append(buffer, bytesRead);
            // std::cout << "PREMIER REQUEST DATA: " << requestData << std::endl;
            // std::cout << "FIN DU PREMIER REQUEST DATA " << std::endl;
            if (requestData.find("\r\n\r\n") != std::string::npos)
                break;
        }

        if (bytesRead <= 0) {
            // std::cout << "No bytes read or error, closing client socket: " << event.data.fd << std::endl;
            compareClientSocket(event.data.fd, 1);
        }
        else {
            Requests req(requestData, this->_servers, serverSocket);

            std::string bodyData;

            // Print headers and potential start of the body
            if (requestData.find("\r\n\r\n") != std::string::npos) {
                std::string headers = requestData.substr(0, requestData.find("\r\n\r\n"));
                std::string potentialBodyStart = requestData.substr(requestData.find("\r\n\r\n") + 4);

                // std::cout << "Headers (hex): " << toHex(headers) << std::endl;
                // std::cout << "Potential Body Start (hex): " << toHex(potentialBodyStart) << std::endl;

                // If part of the body is already in requestData, append it to bodyData
                if (potentialBodyStart.size() > 0) {
                    // std::cout << "Part of the body is already in requestData, size: " << potentialBodyStart.size() << std::endl;
                    bodyData.append(potentialBodyStart);
                }
            }

            // std::cout << "Processing multipart/form-data" << std::endl;
            size_t contentLength = atoi(req.getContentLength().c_str());
            size_t totalBytesRead = bodyData.size(); // Start with any data already in bodyData
            // std::cout << "Content-Length: " << contentLength << std::endl;

            // Read the remaining part of the body if needed
            while (totalBytesRead < contentLength) {
                int remainingBytes = contentLength - totalBytesRead;
                int bytesToRead = std::min(BUF_SIZE, remainingBytes);
                bytesRead = recv(event.data.fd, buffer, bytesToRead, 0);
                
                if (bytesRead <= 0) {
                    std::cerr << "Error reading from socket: " << strerror(errno) << std::endl;
                    compareClientSocket(event.data.fd, 1);
                    return;
                }

                bodyData.append(buffer, bytesRead);
                totalBytesRead += bytesRead;

                // std::cout << "Actual body size: " << bodyData.size() << std::endl;
                // std::cout << "Expected Content-Length: " << contentLength << std::endl;
                // std::cout << "bodyData: " << bodyData << std::endl;
                // std::cout << "Bytes read: " << bytesRead << ", Total bytes read: " << totalBytesRead << std::endl;
            }

            // std::cout << "Total body data read: " << totalBytesRead << " bytes" << std::endl;

            // if (totalBytesRead != contentLength) {
            //     std::cerr << "Warning: Only " << totalBytesRead
            //               << " bytes were read out of expected " << contentLength << " bytes." << std::endl;
            // }

            // std::cout << "AVANT RECEIVED BODY" << std::endl;
            req.receiveBody(bodyData);
            std::string response = req.getResponse();
            // std::cout << "CALLING REPONSE: " << response << std::endl;
            if (response == "") {
                // std::cout << "Empty response, closing client socket: " << event.data.fd << std::endl;
                compareClientSocket(event.data.fd, 1);
            } 
            else {
                // std::cout << "Sending response to client" << std::endl;
                send(event.data.fd, response.c_str(), response.size(), 0);
            }
        }
    }
}

// void ServerManager::handleClientSocket(epoll_event event)
// {
//     char buffer[BUF_SIZE] = {0};
//     int serverSocket = compareClientSocket(event.data.fd, 0);
//     if (serverSocket == -1) {
//         // Si `compareClientSocket` a retourné -1, cela signifie que le socket n'a pas été trouvé
//         std::cerr << "Socket not found or already closed: " << event.data.fd << std::endl;
//         return;  // Ne continuez pas à traiter cet événement
//     }
//     if (event.events & EPOLLHUP)
//     {
//         std::cout << "EPOLLHUP event, closing socket: " << event.data.fd << std::endl;
//         //close(event.data.fd);
//         compareClientSocket(event.data.fd, true); 
//         return;
//     } 
//     else if (event.events & EPOLLIN)
//     {
//         std::string requestData;
//         int bytesRead;

//         while ((bytesRead = recv(event.data.fd, buffer, BUF_SIZE, 0)) > 0) 
//         {
//             //buffer[bytesRead] = '\0';
//             requestData.append(buffer);
//             std::cout << "PREMIER REQUEST DATA: " << requestData << std::endl;
//             std::cout << "FIN DU PREMIER REQUEST DATA " << std::endl;
//             //bzero(buffer, sizeof(buffer));
//             if (requestData.find("\r\n\r\n") != std::string::npos)
//             {
//                  break;
//             }
//         }

//         if (bytesRead <= 0)
//         {
//             std::cout << "No bytes read or error, closing client socket: " << event.data.fd << std::endl;
//             compareClientSocket(event.data.fd, 1);
//         }
//         else
//         {
//             // std::cout << "Request data received: " << requestData << std::endl;
//             // std::cout << "Request data (hex): " << toHex(requestData) << std::endl;
//             Requests req(requestData, this->_servers, serverSocket);

//             std::string bodyData;
//             // if (req.getRequestContentType().find("multipart/form-data") != std::string::npos) 
//             // {
//                 std::cout << "Processing multipart/form-data" << std::endl;
//                 size_t contentLength = atoi(req.getContentLength().c_str());
//                 size_t totalBytesRead = 0;
//                 std::cout << "Content-Length: " << contentLength << std::endl;

//                 while (totalBytesRead < contentLength) 
//                 {
//                     int remainingBytes = contentLength - totalBytesRead;
//                     int bytesToRead = std::min(BUF_SIZE, remainingBytes);
//                     int bytesRead = recv(event.data.fd, buffer,bytesToRead, 0);
//                     // std::cout << "Buffer content (raw): " << std::string(buffer, bytesRead) << std::endl;
//                     // std::string hexBuffer = toHex(std::string(buffer, bytesRead));
//                     // std::cout << "BUFFER AFTER RECV: " << hexBuffer << std::endl;
//                     bodyData.append(buffer, bytesRead);
//                     std::cout << "Actual body size: " << bodyData.size() << std::endl;
//                     std::cout << "Expected Content-Length: " << contentLength << std::endl;
//                     std::cout << "bodyData: " << bodyData << std::endl;
//                     totalBytesRead += bytesRead;
//                     if (bytesRead <= 0) 
//                     {
//                         std::cerr << "Error reading from socket: " << strerror(errno) << std::endl;
//                         compareClientSocket(event.data.fd, 1);
//                         return ;
//                     }
//                     std::cout << "Bytes read: " << bytesRead << ", Total bytes read: " << totalBytesRead << std::endl;
//                     // std::cout << "Bytes read: " << bytesRead << ", Total bytes read: " << totalBytesRead << std::endl;
//                     std::cout << "BUFFER AFTER RECV: " << buffer << std::endl;
//                 }

//                 std::cout << "Total body data read: " << totalBytesRead << " bytes" << std::endl;
    

//             // }
//             if (totalBytesRead != contentLength) {
//                 std::cerr << "Warning: Only " << totalBytesRead 
//                         << " bytes were read out of expected " << contentLength << " bytes." << std::endl;
//             }

//             std::cout << "AVANT RECEIVED BODY" << std::endl;
//             req.receiveBody(bodyData);
//             std::string response = req.getResponse();
//             std::cout << "CALLING REPONSE: " << response << std::endl;
//             if (response == "") 
//             {
//                 std::cout << "Empty response, closing client socket: " << event.data.fd << std::endl;
//                 compareClientSocket(event.data.fd, 1);
//             } 
//             else 
//             {
//                 std::cout << "Sending response to client" << std::endl;
//                 send(event.data.fd, response.c_str(), response.size(), 0);
//             }
//         }
//     }
// }



/*
void ServerManager::handleClientSocket(epoll_event event) {
	char buffer[BUF_SIZE] = {0};
	int serverSocket = compareClientSocket(event.data.fd, 0);
	if (event.events & EPOLLHUP) {
		close(event.data.fd);
		return ;
	}
	else if (event.events & EPOLLIN) {
		std::cout << "serverSocket : " << serverSocket << std::endl;
		std::string requestData;
		int bytesRead;
		while ((bytesRead = recv(event.data.fd, buffer, BUF_SIZE, 0)) > 0) {
			buffer[bytesRead] = '\0';
			requestData.append(buffer);
			bzero(buffer, sizeof(buffer));
			if (requestData.find("\r\n\r\n") != std::string::npos)
				break;
		
		}

		if (bytesRead <= 0)
			compareClientSocket(event.data.fd, 1);
		else {
			std::cout << requestData << std::endl;
			std::cout << "BUFFER size = " << strlen(buffer) << std::endl;
			std::cout << "========================DELIM==================================" << std::endl;
			Requests req(requestData, this->_servers, serverSocket);
			std::string bodyData;
			int bytesRead;
			if (!req.getRequestContentType().compare(0, 19, "multipart/form-data")) {
				while ((bytesRead = recv(event.data.fd, buffer, BUF_SIZE, 0)) > 0) {
					buffer[bytesRead] = '\0';
					std::cout << "BUFFER AFTER RECV " << buffer << std::endl;
					bodyData.append(buffer);
					std::cout << "find = " << bodyData.find(req.getRequestContentType().substr(31, req.getRequestContentType().size())) << std::endl;
					std::cout <<  "getrequestcontentblabla = " << req.getRequestContentType().substr(31, req.getRequestContentType().size()) << std::endl;
					bzero(buffer, sizeof(buffer));
					if (bodyData.find(req.getRequestContentType().substr(31, req.getRequestContentType().size())) != std::string::npos)
						if (bodyData.find(req.getRequestContentType().substr(31, req.getRequestContentType().size()), req.getRequestContentType().size()) != std::string::npos)
							break;
				}
				if (bytesRead <= 0)
					compareClientSocket(event.data.fd, 1);
			}
			std::cout << "LES BIG BOSS" << std::endl;
			req.receiveBody(bodyData);
			std::string response = req.getResponse();
			if (response == "")
				compareClientSocket(event.data.fd, 1);
			else
				send(event.data.fd, response.c_str(), response.size(), 0);
		}
	}
}
*/





std::vector<Server> ServerManager::getServers() const {return this->_servers;}
const char* ServerManager::SocketFailed::what() const throw() {return "Error: Socket Failed !";}
