#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>
#include <vector>

// Structure to hold a single multipart form field
struct	MultipartPart
{
	std::string	name;						// Field name
	std::string	filename;					// Original filename (empty if not a file)
	std::string	content_type;				// Content-Type of this part
	std::string	content_transfer_encoding;	// e.g., "binary", "base64", "7bit"
	std::string	data;						// The actual content/data
	bool		is_file;					// True if this part is a file upload
	
	MultipartPart() : is_file(false) {}
};

class	Request
{
	private:
		std::string							method;				// GET, POST, DELETE
		std::string							path;				// /index.html
		std::string							version;			// HTTP/1.1
		std::map<std::string, std::string>	headers;
		std::string							body;
		std::string							raw_data;			// Accumulated raw data
		bool								headers_complete;	// Have we parsed all headers?
		bool								body_complete;		// Is the full body received?
		size_t								content_length;		// Expected body size
		bool								is_chunked;			// Is Transfer-Encoding: chunked?
		bool								parse_error;		// Was there a parse error?
		int									error_code;			// HTTP error code if parse_error is true

		// Multipart data
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

		// Incremental parsing for non-blocking I/O
		void								appendData(const std::string& data);
		bool								parseHeaders();
		bool								isHeadersComplete() const { return headers_complete; }
		bool								isComplete() const { return headers_complete && body_complete; }
		bool								hasParseError() const { return parse_error; }
		int									getErrorCode() const { return error_code; }
		
		// Getters
		std::string							getMethod() const { return method; }
		std::string							getPath() const { return path; }
		std::string							getBody() const { return body; }
		std::string							getHeader(const std::string& key) const;
		size_t								getContentLength() const { return content_length; }
		
		// For multipart parsing
		std::string							getBoundary() const;
		bool								isMultipart() const;
		bool								parseMultipart();
		const std::vector<MultipartPart>&	getParts() const { return multipart_parts; }
		
		// Utility functions
		static std::string					urlDecode(const std::string& str);
		static std::string					base64Decode(const std::string& str);
		static std::string					quotedPrintableDecode(const std::string& str);
};

#endif