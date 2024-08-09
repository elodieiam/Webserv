
#include "../includes/Requests.hpp"

std::string itostr(int nb) {
	std::stringstream ss;
	std::string str;
	ss << nb;
	ss >> str;
	return str;
}

std::string Requests::getContentLength() const {
    for (std::map<std::string, std::string>::const_iterator it = this->_headers.begin(); it != this->_headers.end(); ++it) {
        std::cout << it->first << ": " << it->second << std::endl;
    }
    std::map<std::string, std::string>::const_iterator it = this->_headers.find("Content-Length");
    if (it != this->_headers.end()) {
        return it->second;
    }
    return "";
}


void Requests::receiveBody(const std::string &buffBody) {
	this->_body = buffBody;
	std::cout << _body << std::endl;
}

std::vector<std::string> split(std::string buf, const std::string &find) {
	std::vector<std::string> vector;
	if (buf.find(find) == std::string::npos) {
		vector.push_back(buf.substr(0, buf.size()));
		return vector;
	}
	for (int i = 0; !buf.empty(); i++) {
		vector.push_back(buf.substr(0, buf.find(find)));
		buf.erase(0, vector[i].size() + 1);
	}
	return vector;
}


bool isSyntaxGood(std::vector<std::string> &request) {
    size_t find;
    bool body = 0;
    bool length = 0;
    bool chunked = 0;

    if (request.empty() || request[0].empty() || split(request[0], " ").size() != 3) {
        return false;
    }

    for (size_t i = 0; i < request.size(); i++) {
        if (body == 0) {
            if (request[i].find("Content-Type: multipart/form-data") != std::string::npos) {
                return true;
            }

            if (!request[i].compare(0, 15, "Content-Length:")) {
                if (length == 1 || chunked == 1) {
                    return false;
                }
                length = 1;
            } else if (!request[i].compare(0, 26, "Transfer-Encoding: chunked")) {
                if (chunked == 1 || length == 1) {
                    return false;
                }
                chunked = 1;
            } else if (!request[i].compare("\r")) {
                body = 1;
                continue;
            }

            find = request[i].find("\r");
            if (find == std::string::npos) {
                return false;
            }
            request[i].erase(find);
            find = request[i].find(": ");
            if (i != 0 && find == std::string::npos) {
                return false;
            }
        } else {
            if (length == 1) {
                find = request[i].find("\r");
                if (find != std::string::npos) {
                    return false;
                }
            } else if (chunked == 1) {
                find = request[i].find("\r");
                if (find == std::string::npos) {
                    return false;
                }
            } else {
                return false;
            }
        }
    }
    return true;
}



std::vector<std::string> getAccept(std::string line) {
	std::vector<std::string> accept = split(line, ",");
	for (unsigned int i = 0; i < accept.size(); i++) {
		size_t find = accept[i].find(";");
		if (find != std::string::npos)
			accept[i].erase(find, accept[i].size());
	}
	for (unsigned int i = 0; i < accept.size(); i++) {
		size_t find = accept[i].find("+");
		size_t find2 = accept[i].find("/");
		if (find != std::string::npos && find2 != std::string::npos) {
			std::string tmp = accept[i].substr(0, find2 + 1);
			accept.push_back(tmp + accept[i].substr(find + 1, accept[i].size()));
			accept[i].erase(find, accept[i].size());
		}
	}
	return accept;
}

void Requests::getQuery() {
	size_t find = this->_path.find("?");
	if (find != std::string::npos) {
		this->_query = this->_path.substr(find + 1, this->_path.size());
		this->_path.erase(find, this->_path.size());

	}
}

Server Requests::findServerWithSocket(std::vector<Server> manager, int serverSocket, std::string serverName) {
	size_t find = serverName.find(":");
	if (find != std::string::npos)
		serverName.erase(find, serverName.size());
	if (serverName == "localhost" || isValidIPAddress(serverName)) {
		for (std::vector<Server>::iterator it = manager.begin(); it != manager.end(); it++)
			if (it->getServerSocket() == serverSocket)
				return *it;
	}
	else
		for (std::vector<Server>::iterator it = manager.begin(); it != manager.end(); it++)
			if (it->getServerSocket() == serverSocket && it->getServerName() == serverName)
				return *it;
	this->_paramValid = 0;
	return manager[0];
}

std::string Requests::getBody(std::vector<std::string> bufSplitted, size_t index, std::map<std::string, std::string> request) {
    std::string contentType = request["Content-Type"];
    
    // Ignorer les vérifications du corps pour multipart/form-data
    if (contentType.find("multipart/form-data") != std::string::npos) {
        std::string body;
        for (size_t i = index + 1; i < bufSplitted.size(); ++i) {
            body += bufSplitted[i] + "\n";
        }
        return body;
    }

    // Ancien traitement pour Content-Length et Transfer-Encoding: chunked
    size_t length = (request.find("Content-Length") != request.end()) ? atoi(request["Content-Length"].c_str()) : 0;
    
    if (length > this->_servParam.getClientMaxBodySize()) {
        this->_statusCode = NOT_IMPLEMENTED;
        std::cout << "Error: Body Length exceeds Client Max Body Size" << std::endl;
        return "";
    }

    if (length != 0) {
        if (index + 1 >= bufSplitted.size() || length != bufSplitted[index + 1].size()) {
            this->_statusCode = BAD_REQUEST;
            std::cout << "Error: Body Length does not match actual length" << std::endl;
            return "";
        }
        return bufSplitted[index + 1];
    }
    return "";
}


std::string Requests::getRootPath(const std::string &path) {
	if (path == "/favicon.ico")
		return "./pages/favicon.ico";
	// std::vector<LocationConfig> tmp = this->_servParam.getLocations();
	// for (size_t i = 0; i < tmp.size(); i++) {
	// 	if (tmp[i].getPath() == path)
	// 		std::cout << "Path found : " << path << std::endl; 
	// }
	// for (size_t i = 0; i < tmp.size(); i++) {
	// 	std::cout << "Path dir : " << tmp[i].getPath() << "?" << std::endl;
	// 	std::cout << "First method : " << tmp[i].getAllowMethods()[0] << "?" << std::endl;
	// 	std::cout << "Index : " << tmp[i].getIndex() << "?" << std::endl;
	// 	std::cout << "Auto index : " << tmp[i].getautoIndex() << "?" << std::endl;
	// 	std::cout << "Root : " << tmp[i].getRoot() << "?" << std::endl;
	// 	std::cout << "Upload dir : " << tmp[i].getUploadDir() << "?" << std::endl;
	// 	std::cout << "Return dir : " << tmp[i].getReturnDirective() << "?" << std::endl;
	// 	std::cout << std::endl;
	// }
	return "." + this->_servParam.getRoot() + path;
}

Requests::Requests(const std::string &buf, std::vector<Server> manager, int serverSocket)
    : _statusCode(OK), _method(""), _path(""), _query(""), _protocol(""), _accept(), _contentType(""), _servParam(), _paramValid(false), _cgiPathPy(""), _cgiPathPhp(""), _body(""), _requestContentType(""), _headers() {
    parseRequest(buf);
    std::vector<std::string> bufSplitted = split(buf, "\n");

    if (!isSyntaxGood(bufSplitted)) {
        std::cout << "Syntax of request is not good" << std::endl;
        this->_paramValid = 1;
        this->_servParam = findServerWithSocket(manager, serverSocket, "localhost");
        this->_statusCode = BAD_REQUEST;
    } else {
        std::map<std::string, std::string> request;
        std::vector<std::string> methodPathProtocol = split(bufSplitted[0], " ");
        request.insert(std::make_pair("Method", methodPathProtocol[0]));
        request.insert(std::make_pair("Path", methodPathProtocol[1]));
        request.insert(std::make_pair("Protocol", methodPathProtocol[2]));
        for (size_t i = 1; i < bufSplitted.size(); i++) {
            if (!bufSplitted[i].compare("\r")) {
                if (i + 1 != bufSplitted.size()) {
                    request.insert(std::make_pair("Body", getBody(bufSplitted, i, request)));
                }
                break;
            }
            request.insert(std::make_pair(bufSplitted[i].substr(0, bufSplitted[i].find(": ")), bufSplitted[i].substr(bufSplitted[i].find(": ") + 2, bufSplitted[i].size())));
        }

        this->_headers = request;
        this->_paramValid = 1;
        this->_servParam = findServerWithSocket(manager, serverSocket, request["Host"]);
        this->_method = _headers["Method"];
        this->_path = getRootPath(request["Path"]);
        this->_protocol = request["Protocol"];
        this->_accept = getAccept(request["Accept"]);
        this->_body = request["Body"];
        if (_headers.find("Content-Type") != _headers.end()) {
            _requestContentType = _headers["Content-Type"];
        }
        getQuery();
        setCgiPathPy(extractCgiPathPy());
        setCgiPathPhp(extractCgiPathPhp());
        if (this->_protocol != "HTTP/1.1") {
            this->_statusCode = HTTP_VERSION_NOT_SUPPORTED;
        } else {
            this->_statusCode = OK;
        }
    }
}



void Requests::parseRequest(const std::string& rawRequest) {
    std::istringstream stream(rawRequest);
    std::string line;

    // Lire la première ligne pour obtenir la méthode, le chemin et le protocole
    if (std::getline(stream, line)) {
        // Supprimer le retour chariot à la fin de la ligne, s'il existe
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }

        // Diviser la ligne de requête en ses composants
        std::vector<std::string> methodPathProtocol = split(line, " ");
        if (methodPathProtocol.size() == 3) {
            this->_method = methodPathProtocol[0];
            this->_path = methodPathProtocol[1];
            this->_protocol = methodPathProtocol[2];
        } else {
            std::cerr << "Warning: Malformed request line: " << line << std::endl;
            this->_statusCode = BAD_REQUEST;
            return;
        }
    }
    std::string headersPart = rawRequest.substr(rawRequest.find("\r\n") + 2);
    size_t headersEnd = headersPart.find("\r\n\r\n");
    if (headersEnd != std::string::npos) {
        parseHeaders(headersPart.substr(0, headersEnd));
    } else {
        std::cerr << "Warning: No headers found in request." << std::endl;
    }
}



void Requests::parseHeaders(const std::string& headersPart) {
    std::istringstream stream(headersPart);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }
        if (line.empty())
            break;
        size_t delimiterPos = line.find(": ");
        if (delimiterPos != std::string::npos) {
            std::string headerName = line.substr(0, delimiterPos);
            std::string headerValue = line.substr(delimiterPos + 2);

            std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);
            if (_headers.find(headerName) != _headers.end()) {
                _headers[headerName] += ", " + headerValue;
            } else {
                _headers[headerName] = headerValue;
            }
        } else {
            std::cerr << "Warning: Malformed header line: " << line << std::endl;
        }
    }
}


void Requests::parseMultipartPart(const std::string& part) {

    size_t headerEnd = part.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return;
    std::string headersPart = part.substr(0, headerEnd);
    std::string contentPart = part.substr(headerEnd + 4);

    std::istringstream stream(headersPart);
    std::string line;
    std::map<std::string, std::string> partHeaders;
    while (std::getline(stream, line) && line != "\r") {
        size_t delimiterPos = line.find(": ");
        if (delimiterPos != std::string::npos) {
            std::string headerName = line.substr(0, delimiterPos);
            std::string headerValue = line.substr(delimiterPos + 2);
            partHeaders[headerName] = headerValue;
        }
    }
    if (partHeaders["Content-Disposition"].find("filename=") != std::string::npos) {
        std::string filename = partHeaders["Content-Disposition"].substr(partHeaders["Content-Disposition"].find("filename=") + 10);
        filename = filename.substr(0, filename.size() - 1);


        std::ofstream file(("uploads/" + filename).c_str(), std::ios::binary);
        file.write(contentPart.c_str(), contentPart.size());
        file.close();
    }
	else {
        std::string fieldName = partHeaders["Content-Disposition"].substr(partHeaders["Content-Disposition"].find("name=") + 6);
        fieldName = fieldName.substr(0, fieldName.size() - 1);
    }
}


void Requests::processMultipartFormData(const std::string& bodyData) {

    std::string boundary = "--" + _headers["Content-Type"].substr(_headers["Content-Type"].find("boundary=") + 9);
    size_t start = bodyData.find(boundary);
    while (start != std::string::npos) {
        start += boundary.size();
        if (bodyData.substr(start, 2) == "--")
            break;
        size_t end = bodyData.find(boundary, start);
        if (end == std::string::npos)
            break;
        std::string part = bodyData.substr(start, end - start);
        parseMultipartPart(part);
        start = end;
    }
}

void Requests::handlePostRequest() {
    size_t contentLength = static_cast<size_t>(atoi(_headers["Content-Length"].c_str()));
    size_t totalBytesRead = 0;
    std::string bodyData;

    while (totalBytesRead < contentLength) {
        char buffer[1024];
        ssize_t bytesRead = recv(_clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0)
            break;
        bodyData.append(buffer, bytesRead);
        totalBytesRead += bytesRead;
    }

    if (totalBytesRead < contentLength) {
        this->_statusCode = BAD_REQUEST;
        return;
    }
    processMultipartFormData(bodyData);
}


Requests::~Requests() {}

bool Requests::checkExtension() {
	if (this->_path == "./pages/favicon.ico") {
		this->_contentType = "image/x-icon";
		return true;
	}
	size_t find = this->_path.find_last_of(".");
	if (find != std::string::npos) {
		std::string extension = this->_path.substr(find + 1, this->_path.size());
		if (extension == "php" || extension == "py") {
			this->_contentType = "text/html";
			return true;
		}
		if (extension == "css") {
            this->_contentType = "text/css";
            return true;
        }
        else if (extension == "html" || extension == "htm") {
            this->_contentType = "text/html";
            return true;
        }
		for (unsigned int i = 0; i < this->_accept.size(); i++) {
			size_t find = this->_accept[i].find("/");
			if (find != std::string::npos) {
				std::string type = this->_accept[i].substr(find + 1, this->_accept[i].size());
				if (extension == type) {
					this->_contentType = this->_accept[i];
					return true;
				}
			}
		}
	}
	return false;
}

void Requests::checkPage() {
	DIR *dir = opendir(this->_path.c_str());
	if (dir != NULL) {
		closedir(dir);
		std::string slash = "";
		if (this->_path[this->_path.size() - 1] != '/')
			slash = "/";
		this->_path = this->_path + slash + "index.html";
	}
	if (access(this->_path.c_str(), F_OK))
		this->_statusCode = NOT_FOUND;
	else if (access(this->_path.c_str(), R_OK))
		this->_statusCode = FORBIDDEN;
	else if (!checkExtension())
		this->_statusCode = NOT_ACCEPTABLE;
}

std::string getPage(std::string filename, std::string responseStatus) {
	std::ifstream fd(filename.c_str());
	if (!fd.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
    }
    std::string line;
	std::string response = responseStatus;
    while (getline(fd, line))
        response.append(line + "\n");
    fd.close();
	return response;
}

std::string Requests::readFromPipe(int pipeFd) {
	char buffer[4096];
	ssize_t bytesRead = read(pipeFd, buffer, 4096 - 1);
	if (bytesRead == -1) {
		this->_statusCode = INTERNAL_SERVER_ERROR;
		return "";
	}
	std::string result;
	while (bytesRead > 0) {
		buffer[bytesRead] = '\0';
		result += buffer;
		bytesRead = read(pipeFd, buffer, 4096 - 1);
		if (bytesRead == -1) {
			this->_statusCode = INTERNAL_SERVER_ERROR;
			return "";
		}
	}
	return result;
}

char **Requests::vectorToCharArray(const std::vector<std::string> &vector) {
	char** arr = new char*[vector.size() + 1];
	for (size_t i = 0; i < vector.size(); i++) {
		arr[i] = new char[vector[i].size() + 1];
		std::copy(vector[i].begin(), vector[i].end(), arr[i]);
		arr[i][vector[i].size()] = '\0';
	}
	arr[vector.size()] = NULL;
	return arr;
}

static void exportVar(std::vector<std::string> &env, const std::string&key, const std::string& value) {env.push_back(key + '=' + value);}

std::vector<std::string> Requests::createCgiEnv() {
	std::vector<std::string> env;
	exportVar(env, "REQUEST_METHOD", this->_method);
	exportVar(env, "SCRIPT_NAME", "response.php");
	exportVar(env, "SERVER_PROTOCOL", "HTTP/1.1");
	exportVar(env, "SERVER_SOFTWARE", "webserv/1.1");
	// exportVar(env, "DOCUMENT_ROOT", "/var/www/html");
	if (!this->_method.compare("GET"))
		exportVar(env, "QUERY_STRING", this->_query);
	else if (!this->_method.compare("POST")) {
		exportVar(env, "CONTENT_LENGTH", itostr(this->_body.size()));
		exportVar(env, "CONTENT_TYPE", "application/x-www-form-urlencoded"); //todo _contentTypeRequest
	}
	return env;
}

std::string Requests::getRequestContentType() const {return this->_requestContentType;}


std::string  Requests::execCgi(const std::string& scriptType) 
{
	int childPid;
	int childValue;
	int fd[2];
	int fdBody[2];
	const char *scriptInterpreter;
	if (pipe(fd) == -1)
		return this->_statusCode = 500, setErrorPage();
	childPid = fork();
	if (childPid == -1)
		return this->_statusCode = 500, setErrorPage();
	//si POST, on crée un | pour permettre l'écriture et la lecture du body
	if (!this->_method.compare("POST")) {
		if (pipe(fdBody) == -1)
			return this->_statusCode = 500, setErrorPage();
		if (write(fdBody[1], this->_body.c_str(), static_cast<int>(this->_body.size())) == -1)
			return this->_statusCode = 500, setErrorPage();
	}
	if (!childPid) {
		if (!this->_method.compare("POST")) {
			close(fdBody[1]);
			if (dup2(fdBody[0], STDIN_FILENO))
				exit(EXIT_FAILURE);
			close(fdBody[0]);
		}
		close(fd[0]);
		if (dup2(fd[1], STDOUT_FILENO) == -1) 
			exit(EXIT_FAILURE);
		close(fd[1]);
		if (!scriptType.compare("py"))
			scriptInterpreter = this->_cgiPathPy.c_str();
		else
			scriptInterpreter = this->_cgiPathPhp.c_str();
		char **env = vectorToCharArray(createCgiEnv());
		char *args[] = { const_cast<char*>(scriptInterpreter), const_cast<char*>(_path.c_str()), NULL };
		execve(scriptInterpreter, args, env);
		delete [] env;
		exit(EXIT_FAILURE);
	}
	if (waitpid(childPid, &childValue, WUNTRACED) == -1)
			return close(fd[0]), close(fd[1]), this->_statusCode = 500, setErrorPage();
	if (WEXITSTATUS(childValue) == 1)
		return close(fd[0]), close(fd[1]), this->_statusCode = 500, setErrorPage();
	if (!this->_method.compare("POST")) {
		close(fdBody[0]);
		close(fdBody[1]);
	}
	close(fd[1]); 
    std::string scriptContent = readFromPipe(fd[0]);
	close(fd[0]);
	std::string line;
	if (this->_statusCode != OK)
		return setErrorPage();
	std::string response = "HTTP/1.1 200 OK\n\n";
        response.append(scriptContent + "\n");
	return setResponseScript(scriptContent, "OK") + scriptContent;
	return response;
}

std::string Requests::doUpload() 
{

    // Extraire la boundary du Content-Type
    std::string boundary = "--" + this->_requestContentType.substr(this->_requestContentType.find("boundary=") + 9);
 

    // Nettoyer la boundary des caractères indésirables
    if (!boundary.empty() && (boundary[boundary.size() - 1] == '\r' || boundary[boundary.size() - 1] == '\n')) {
        boundary = boundary.substr(0, boundary.size() - 1);
    }

    // Trouver le début de la première partie après la boundary
    size_t pos = this->_body.find(boundary);
    if (pos == std::string::npos) {
        std::cerr << "Boundary not found" << std::endl;
        this->_statusCode = BAD_REQUEST;
        return setErrorPage();
    }

    pos += boundary.size();  // Aller après la boundary

    // Recherche de la boundary de fin
    std::string endBoundary = boundary + "--";

    size_t end = this->_body.find(endBoundary, pos);
    if (end == std::string::npos) {
        std::cerr << "End boundary not found" << std::endl;
        std::cerr << "Searching for: " << endBoundary << std::endl;
        std::cerr << "In content: " << this->_body.substr(pos) << std::endl;
        this->_statusCode = BAD_REQUEST;
        return setErrorPage();
    }

    std::string part = this->_body.substr(pos, end - pos);

    size_t headerEnd = part.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        std::cerr << "Headers not found in the part" << std::endl;
        this->_statusCode = BAD_REQUEST;
        return setErrorPage();
    }

    std::string headersPart = part.substr(0, headerEnd);
    std::string contentPart = part.substr(headerEnd + 4); // Aller après \r\n\r\n pour le contenu

    // Extraction du nom de fichier
    size_t filenamePos = headersPart.find("filename=\"");
    if (filenamePos == std::string::npos) {
        std::cerr << "Filename not found in headers" << std::endl;
        this->_statusCode = BAD_REQUEST;
        return setErrorPage();
    }

    filenamePos += 10; // Aller après `filename="`
    size_t filenameEnd = headersPart.find("\"", filenamePos);
    if (filenameEnd == std::string::npos) {
        std::cerr << "Filename end quote not found" << std::endl;
        this->_statusCode = BAD_REQUEST;
        return setErrorPage();
    }

    // std::string filename = headersPart.substr(filenamePos, filenameEnd - filenamePos);
    // std::string filePath = "upload/" + filename;

	// int count = 1;
    // std::string newFilePath = filePath;
    // size_t dotPosition = filename.find_last_of(".");
    // std::string baseName = filename.substr(0, dotPosition);
    // std::string extension = (dotPosition != std::string::npos) ? filename.substr(dotPosition) : "";

    // while (std::ifstream(newFilePath.c_str()).is_open()) {
    //     std::stringstream ss;
    //     ss << "uploads/" << baseName << "_" << count++ << extension;
    //     newFilePath = ss.str();
    // }

    // // Créez ou ouvrez le fichier dès maintenant pour l'écriture
    // std::ofstream outFile(filename.c_str());
    // if (!outFile.is_open()) {
    //     std::cerr << "Failed to open file for writing" << std::endl;
    //     this->_statusCode = INTERNAL_SERVER_ERROR;
    //     return setErrorPage();
    // }
	std::string filename = headersPart.substr(filenamePos, filenameEnd - filenamePos);
    std::string filePath = "upload/" + filename;

    // Vérifiez si le fichier existe déjà et ajoutez un suffixe numérique si nécessaire
    int count = 1;
    std::string newFilePath = filePath;
    size_t dotPosition = filename.find_last_of(".");
    std::string baseName = filename.substr(0, dotPosition);
    std::string extension = (dotPosition != std::string::npos) ? filename.substr(dotPosition) : "";

    while (std::ifstream(newFilePath.c_str()).is_open()) {
        std::stringstream ss;
        ss << "upload/" << baseName << "_" << count++ << extension;
        newFilePath = ss.str();
    }

    std::cout << "Final file path: " << newFilePath << std::endl;

    // Enregistrez le contenu du fichier
    std::ofstream outFile(newFilePath.c_str(), std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open file for writing" << std::endl;
        this->_statusCode = INTERNAL_SERVER_ERROR;
        return setErrorPage();
    }

    outFile.write(contentPart.c_str(), contentPart.size());
    outFile.close();

    std::cout << "File uploaded successfully" << std::endl;

    this->_statusCode = OK;
    return setResponse("File uploaded successfully");
}






std::string Requests::getResponse() {

    if (!this->_paramValid) 
        return "";
    if (this->_statusCode != OK)
        return setErrorPage();

    std::vector<std::string> words = split(this->_path, ".");
    if (words.size() == 1) {
        this->_statusCode = BAD_REQUEST;
        return setErrorPage();
    }
    if (words.back() == "py" || words.back() == "php") {
        if (access(this->_path.c_str(), X_OK)) {
            this->_statusCode = FORBIDDEN;
            return setErrorPage();
        }
        return execCgi(words.back());
    }
    if (this->_method == "POST" && this->_requestContentType.find("multipart/form-data") != std::string::npos)
        return doUpload();
    if (this->_contentType.empty())
        this->_contentType = "text/html";
    return getPage(this->_path, setResponse("OK"));
}




std::string Requests::setErrorPage() {
	ErrorPage err = this->_servParam.getErrorPage();
	this->_protocol = err.getProtocol();
	this->_contentType = err.getContentType();
	this->_path = err.getPath(this->_statusCode);
	return getPage(this->_path, setResponse(err.getErrorMessage(this->_statusCode)));
}

std::string Requests::setResponse(const std::string &codeName) {
	std::string response;
	struct stat buf;
	int st = stat(this->_path.c_str(), &buf);
	response.append(this->_protocol + " " + itostr(this->_statusCode) + " " + codeName + "\n");
	response.append("Connection: Keep-Alive\n");
	if (st != -1) {
		size_t bytes = buf.st_size;
		response.append("Content-Length: ");
		response.append(itostr(bytes));
		response.append("\n");
	}
	response.append("Content-Location: " + this->_path.substr(1, this->_path.size()) + "\n");
	response.append("Content-Type: " + this->_contentType + "\n");
	if (st != -1) {
		response.append("Date: ");
		response.append(ctime(&buf.st_atime));
		response.append("Last-Modified: ");
		response.append(ctime(&buf.st_mtime));
	}
	response.append("Server: Webserv\n");
	response.append("\n");
	return response;
}

std::string Requests::setResponseScript(const std::string &scriptResult, const std::string &codeName) {
	std::string response;
	response.append(this->_protocol + " " + itostr(this->_statusCode) + " " + codeName + "\n");
	response.append("Connection: Keep-Alive\n");
	response.append("Content-Length: ");
	response.append(itostr(strlen(scriptResult.c_str())));
	response.append("\n");
	response.append("Content-Location: " + this->_path.substr(1, this->_path.size()) + "\n");
	response.append("Content-Type: " + this->_contentType + "\n");
	response.append("Server: Webserv\n");
	response.append("\n");
	return response;
}

std::string Requests::extractCgiPathPy() const {
	std::vector<LocationConfig> locations = this->_servParam.getLocations();
	std::string cgiPath;
	for (size_t i = 0; i < locations.size(); i++) {
		if (locations[i].getPath() == "/cgi-bin") {
			 std::map<std::string, std::string> cgiPaths = locations[i].getCgiPaths();
			 std::map<std::string, std::string>::iterator it = cgiPaths.find(".py");
            cgiPath = it->second;
			}
		}
	return cgiPath;
}

void Requests::setCgiPathPy(const std::string &path) {this->_cgiPathPy = path;}
std::string Requests::getCgiPathPy() const {return this->_cgiPathPy;}

std::string Requests::extractCgiPathPhp() const {
	std::vector<LocationConfig> locations = this->_servParam.getLocations();
	std::string cgiPath;
	for (size_t i = 0; i < locations.size(); i++) {
		if (locations[i].getPath() == "/cgi-bin") {
			 std::map<std::string, std::string> cgiPaths = locations[i].getCgiPaths();
			 std::map<std::string, std::string>::iterator it = cgiPaths.find(".php");
            cgiPath = it->second;
			}
		}
	return cgiPath;
}

void Requests::setCgiPathPhp(const std::string &path) {this->_cgiPathPhp = path;}
std::string Requests::getCgiPathPhp() const {return this->_cgiPathPhp;}