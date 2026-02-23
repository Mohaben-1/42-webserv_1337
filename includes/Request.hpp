#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>
#include <vector>

struct	MultipartPart
{
	std::string	name;
	std::string	filename;
	std::string	content_type;
	std::string	content_transfer_encoding;
	std::string	data;
	bool		is_file;
	
	MultipartPart() : is_file(false) {}
};

class	Request
{
	private:
		std::string							method;
		std::string							path;
		std::string							version;
		std::map<std::string, std::string>	headers;
		std::string							body;
		std::string							raw_data;
		bool								headers_complete;
		bool								body_complete;
		size_t								content_length;
		bool								is_chunked;
		bool								parse_error;
		int									error_code;

		std::vector<MultipartPart>			multipart_parts;
		bool								multipart_parsed;

		void		parseContentDisposition(const std::string& header, std::string& name, std::string& filename);
		void		parseContentType(const std::string& header, std::string& mime_type);
		std::string	trim(const std::string& str) const;
		std::string	extractQuotedValue(const std::string& str, const std::string& key) const;
		std::string	extractUnquotedValue(const std::string& str, const std::string& key) const;
		bool		findBoundaryPosition(const std::string& data, const std::string& boundary, size_t start, size_t& pos) const;
		std::string	unchunkBody(const std::string& chunked_body) const;
		bool		validateRequestLine();
	public:
		Request();

		void								reset();

		void								appendData(const std::string& data);
		bool								parseHeaders();
		bool								isHeadersComplete() const { return headers_complete; }
		bool								isComplete() const { return headers_complete && body_complete; }
		bool								hasParseError() const { return parse_error; }
		int									getErrorCode() const { return error_code; }

		std::string							getMethod() const { return method; }
		std::string							getPath() const { return path; }
		std::string							getBody() const { return body; }
		std::string							getHeader(const std::string& key) const;
		size_t								getContentLength() const { return content_length; }

		std::string							getBoundary() const;
		bool								isMultipart() const;
		bool								parseMultipart();
		const std::vector<MultipartPart>&	getParts() const { return multipart_parts; }
		
		static std::string					urlDecode(const std::string& str);
		static std::string					base64Decode(const std::string& str);
		static std::string					quotedPrintableDecode(const std::string& str);
};

#endif