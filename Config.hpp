#ifndef CONFIG_HPP
#define CONFIG_HPP
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "LocationConfig.hpp"
#include "ErrorPageConfig.hpp"
#include "ServerConfig.hpp"

class Config 
{
	public:
		Config();
		Config(Config const &other);
		Config &operator=(Config const &other);
		~Config();

		void addServer(const ServerConfig &server);
		std::vector<ServerConfig> getServers() const;
		static std::vector<std::string> split(const std::string &str, char delimiter);
		static Config readConfig(const std::string &filename);
		static Config location(std::vector<std::string> tokens, std::string line, LocationConfig current_location);
		static Config server(std::vector<std::string> tokens, std::string line, ServerConfig current_server);
		static Config error_page(std::vector<std::string> tokens, std::string line, ServerConfig current_server);

	private:
		std::vector<ServerConfig> servers;

	class Openfile : public std::exception
	{
		public:
		virtual const char *what(void) const throw()
		{
			return ("Error: could not open file.");
		}
	};
};

#endif 
