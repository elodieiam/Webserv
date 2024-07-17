/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Requests.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: elrichar <elrichar@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/15 11:57:21 by niromano          #+#    #+#             */
/*   Updated: 2024/07/17 17:14:22 by elrichar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <vector>


class Requests {

	public :

		Requests(const std::string &type, const std::string &path, const std::string &method);
		~Requests();
		std::string getType();
		std::string getPath();
		std::string getMethod();
		std::string getResponse();
		bool isSyntaxError();
		
		//cgi elodie
		std::vector<std::string> split(std::string str, const std::string&  delimiter);
		std::string 				 execCgi(const std::string& scriptType);
		
	private :

		const std::string _type;
		const std::string _path;
		const std::string _method;

};

Requests readRequest(std::string buf);