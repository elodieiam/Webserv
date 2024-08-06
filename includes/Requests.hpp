#pragma once

#include <unistd.h>
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "Server.hpp"
#include "Config.hpp"

enum StatusCode {
	OK = 200,
	MOVED_PERMANENTLY = 301,
	BAD_REQUEST = 400,
	FORBIDDEN = 403,
	NOT_FOUND = 404,
	METHOD_NOT_ALLOWED = 405,
	NOT_ACCEPTABLE = 406,
	INTERNAL_SERVER_ERROR = 500,
	NOT_IMPLEMENTED = 501,
	HTTP_VERSION_NOT_SUPPORTED = 505
};

class Requests {

	public :

		Requests(const std::string &buf, std::vector<Server> manager, int serverSocket);
		~Requests();
		std::string getResponse();

	private :

		int _statusCode;
		std::string _method;
		std::string _path;
		std::string _query;
		std::string _protocol;
		std::vector<std::string> _accept;
		std::vector<std::string> _allowMethod;
		std::string _contentType;
		Server _servParam;
		bool _paramValid;
		std::string _cgiPathPy;
		std::string _cgiPathPhp;
		std::string _body;

		void getQuery();
		void checkPage();
		bool checkExtension();
		std::string setErrorPage();
		std::string setResponse(const std::string &codeName);
		Server findServerWithSocket(std::vector<Server> manager, int serverSocket, std::string serverName);
		std::string getBody(std::vector<std::string> bufSplitted, size_t i, std::map<std::string, std::string> request);
		std::string getRootPath(const std::string &path);

		std::string execCgi(const std::string& scriptType);
		std::string readFromPipe(int pipeFd);
		char** vectorToCharArray(const std::vector<std::string> &vector);
	    std::vector<std::string> createCgiEnv();
		std::string getCgiPathPy() const;
		std::string extractCgiPathPy() const;
		std::string extractCgiPathPhp() const;
		std::string setResponseScript(const std::string &scriptResult, const std::string &codeName);
		void setCgiPathPy(const std::string &path);
		std::string getCgiPathPhp() const;
		void setCgiPathPhp(const std::string &path);
};

std::string itostr(int nb);