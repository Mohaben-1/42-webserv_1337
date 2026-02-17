#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

class	Response
{
	private:
		int									status_code;
		std::string							status_message;
		std::map<std::string, std::string>	headers;
		std::string							body;
	public:
		Response();

		void				setStatus(int code, const std::string& message);
		void				setHeader(const std::string& key, const std::string& value);
		void				setBody(const std::string& content);
		int					getStatusCode() const;
		std::string			toString() const;
		
		// Helper function to get Content-Type from file extension
		static std::string	getContentType(const std::string& path);
};

#endif