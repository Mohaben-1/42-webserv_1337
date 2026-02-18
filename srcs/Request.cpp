#include "../includes/Request.hpp"
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>

// Base64 decoding table
static const std::string	base64_chars = 
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

static inline bool	is_base64(unsigned char c)
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

Request::Request() : headers_complete(false), body_complete(false), content_length(0), is_chunked(false), parse_error(false), error_code(0), multipart_parsed(false) {}

void Request::reset()
{
	method.clear();
	path.clear();
	version.clear();
	headers.clear();
	body.clear();
	raw_data.clear();
	headers_complete = false;
	body_complete = false;
	content_length = 0;
	is_chunked = false;
	parse_error = false;
	error_code = 0;
	multipart_parts.clear();
	multipart_parsed = false;
}

std::string	Request::trim(const std::string& str) const
{
	size_t	start = 0;
	size_t	end = str.length();

	while (start < end && (str[start] == ' ' || str[start] == '\t' || str[start] == '\r' || str[start] == '\n'))
		start++;
	while (end > start && (str[end-1] == ' ' || str[end-1] == '\t' || str[end-1] == '\r' || str[end-1] == '\n'))
		end--;
	return (str.substr(start, end - start));
}

// URL decode a string (handles %XX encoding and + for spaces)
std::string	Request::urlDecode(const std::string& str)
{
	std::string	result;

	result.reserve(str.length());
	for (size_t i = 0; i < str.length(); i++)
	{
		if (str[i] == '%' && i + 2 < str.length())
		{
			// Decode %XX
			char	hex[3] = {str[i+1], str[i+2], 0};
			char*	end;
			long	val = strtol(hex, &end, 16);

			if (end == hex + 2)
			{
				result += static_cast<char>(val);
				i += 2;
				continue ;
			}
		}
		else if (str[i] == '+')
		{
			result += ' ';
			continue ;
		}
		result += str[i];
	}
	return (result);
}

// Base64 decode
std::string	Request::base64Decode(const std::string& encoded_string)
{
	size_t			in_len = encoded_string.size();
	size_t			i = 0;
	size_t			in_ = 0;
	unsigned char	char_array_4[4], char_array_3[3];
	std::string		ret;

	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
	{
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i == 4)
		{
			for (i = 0; i < 4; i++)
				char_array_4[i] = base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; i < 3; i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i)
	{
		for (size_t j = i; j < 4; j++)
			char_array_4[j] = 0;

		for (size_t j = 0; j < 4; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (size_t j = 0; j < i - 1; j++)
			ret += char_array_3[j];
	}
	return (ret);
}

// Quoted-printable decode
std::string	Request::quotedPrintableDecode(const std::string& str)
{
	std::string	result;

	result.reserve(str.length());
	for (size_t i = 0; i < str.length(); i++)
	{
		if (str[i] == '=' && i + 2 < str.length())
		{
			if (str[i+1] == '\r' && str[i+2] == '\n')
			{
				// Soft line break, skip
				i += 2;
				continue ;
			}
			else if (str[i+1] == '\n')
			{
				// Soft line break (non-standard)
				i += 1;
				continue ;
			}
			// Decode =XX
			char	hex[3] = {str[i+1], str[i+2], 0};
			char*	end;
			long	val = strtol(hex, &end, 16);

			if (end == hex + 2)
			{
				result += static_cast<char>(val);
				i += 2;
				continue ;
			}
		}
		result += str[i];
	}
	return (result);
}

// Decode chunked transfer encoding
// Format: <size_hex>\r\n<data>\r\n...<size_hex>\r\n<data>\r\n0\r\n\r\n
std::string	Request::unchunkBody(const std::string& chunked_body) const
{
	std::string	result;
	size_t		pos = 0;
	
	while (pos < chunked_body.length())
	{
		// Find the end of the chunk size line
		size_t	line_end = chunked_body.find("\r\n", pos);

		if (line_end == std::string::npos)
			break;

		// Parse chunk size (hex)
		std::string	size_str = chunked_body.substr(pos, line_end - pos);
		// Remove any chunk extensions (after semicolon)
		size_t		semi = size_str.find(';');

		if (semi != std::string::npos)
			size_str = size_str.substr(0, semi);

		char*	end_ptr;
		size_t	chunk_size = strtol(size_str.c_str(), &end_ptr, 16);
		
		// If chunk size is 0, we're done
		if (chunk_size == 0)
			break;

		// Move past the size line
		pos = line_end + 2;

		// Extract chunk data
		if (pos + chunk_size <= chunked_body.length())
		{
			result += chunked_body.substr(pos, chunk_size);
			pos += chunk_size;
		}

		// Skip trailing \r\n after chunk data
		if (pos + 2 <= chunked_body.length() && chunked_body[pos] == '\r' && chunked_body[pos + 1] == '\n')
			pos += 2;
	}
	return (result);
}

// Validate request line format: METHOD SP URI SP HTTP/VERSION
// Returns true if valid, false if malformed (sets parse_error and error_code)
bool	Request::validateRequestLine()
{
	// All three components must be present
	if (method.empty() || path.empty() || version.empty())
	{
		parse_error = true;
		error_code = 400;
		return (false);
	}

	// Method must be alphabetic uppercase (RFC 7230)
	for (size_t i = 0; i < method.length(); i++)
	{
		if (method[i] < 'A' || method[i] > 'Z')
		{
			parse_error = true;
			error_code = 400;
			return (false);
		}
	}

	// Path must start with '/' or be '*'
	if (path[0] != '/' && path != "*")
	{
		parse_error = true;
		error_code = 400;
		return (false);
	}

	// Version must match HTTP/x.x pattern
	if (version.length() < 6 || version.substr(0, 5) != "HTTP/")
	{
		parse_error = true;
		error_code = 400;
		return (false);
	}

	std::string	ver_num = version.substr(5);
	if (ver_num.length() < 3 || ver_num[1] != '.')
	{
		parse_error = true;
		error_code = 400;
		return (false);
	}
	if (ver_num[0] < '0' || ver_num[0] > '9' || ver_num[2] < '0' || ver_num[2] > '9')
	{
		parse_error = true;
		error_code = 400;
		return (false);
	}

	// Only HTTP/1.0 and HTTP/1.1 are supported (RFC 7230)
	if (version != "HTTP/1.0" && version != "HTTP/1.1")
	{
		parse_error = true;
		error_code = 505;
		return (false);
	}

	return (true);
}

void	Request::appendData(const std::string& data)
{
	raw_data += data;

	// If headers are already parsed, update body from raw_data
	if (headers_complete)
	{
		size_t	header_end = raw_data.find("\r\n\r\n");

		if (header_end != std::string::npos)
		{
			std::string	raw_body = raw_data.substr(header_end + 4);

			// Check if body is now complete
			if (is_chunked)
			{
				// For chunked encoding, look for terminating 0\r\n\r\n
				if (raw_body.find("0\r\n\r\n") != std::string::npos)
				{
					body = unchunkBody(raw_body);
					body_complete = true;
				}
			}
			else if (content_length == 0 || raw_body.length() >= content_length)
			{
				body_complete = true;
				body = raw_body;

				// Trim body to content_length
				if (content_length > 0 && body.length() > content_length)
					body = body.substr(0, content_length);
			}
			else
				body = raw_body;
		}
	}
}

bool	Request::parseHeaders()
{
	if (headers_complete)
		return (true);

	// Look for end of headers (\r\n\r\n)
	size_t	header_end = raw_data.find("\r\n\r\n");

	if (header_end == std::string::npos)
		return (false);  // Headers not complete yet

	// Parse the headers
	std::string			header_section = raw_data.substr(0, header_end);
	std::istringstream	stream(header_section);
	std::string			line;

	// Parse request line: GET /index.html HTTP/1.1
	if (std::getline(stream, line))
	{
		// Remove \r if present
		if (!line.empty() && line[line.length()-1] == '\r')
			line.erase(line.length()-1);

		std::istringstream	request_line(line);
		request_line >> method >> path >> version;
	}

	// Validate request line
	if (!validateRequestLine())
	{
		headers_complete = true;
		body_complete = true;
		return (true);
	}

	// Parse headers
	while (std::getline(stream, line))
	{
		// Remove \r if present
		if (!line.empty() && line[line.length()-1] == '\r')
			line.erase(line.length()-1);
		
		if (line.empty())
			break ;
		
		size_t	colon = line.find(':');

		if (colon != std::string::npos)
		{
			std::string	key = line.substr(0, colon);
			std::string	value = line.substr(colon + 1);

			// Trim leading space from value
			while (!value.empty() && value[0] == ' ')
				value.erase(0, 1);
			headers[key] = value;
		}
	}

	// Get Content-Length if present
	std::string	cl = getHeader("Content-Length");
	if (!cl.empty())
		content_length = std::atol(cl.c_str());

	// HTTP/1.1 requires Host header (RFC 7230 Section 5.4)
	if (version == "HTTP/1.1" && getHeader("Host").empty())
	{
		parse_error = true;
		error_code = 400;
		headers_complete = true;
		body_complete = true;
		return (true);
	}

	// Check for chunked transfer encoding
	std::string te = getHeader("Transfer-Encoding");
	if (te.find("chunked") != std::string::npos)
		is_chunked = true;
	
	headers_complete = true;
	
	// Extract body (everything after \r\n\r\n)
	body = raw_data.substr(header_end + 4);
	
	// Check if body is complete
	if (is_chunked)
	{
		// For chunked encoding, check for terminating chunk (0\r\n\r\n)
		if (body.find("0\r\n\r\n") != std::string::npos)
		{
			// Unchunk the body
			body = unchunkBody(body);
			body_complete = true;
		}
	}
	else if (content_length == 0 || body.length() >= content_length)
	{
		body_complete = true;
		// Trim body to content_length
		if (content_length > 0 && body.length() > content_length)
			body = body.substr(0, content_length);
	}
	return (true);
}

std::string	Request::getHeader(const std::string& key) const
{
	std::map<std::string, std::string>::const_iterator	it = headers.find(key);

	if (it != headers.end())
		return (it->second);
	return ("");
}

// Extract a quoted value like name="value" or name='value'
std::string	Request::extractQuotedValue(const std::string& str, const std::string& key) const
{
	// Try key="value"
	std::string	search = key + "=\"";
	size_t		pos = str.find(search);

	if (pos != std::string::npos)
	{
		pos += search.length();

		size_t	end = str.find('"', pos);

		if (end != std::string::npos)
			return str.substr(pos, end - pos);
	}
	
	// Try key='value'
	search = key + "='";
	pos = str.find(search);
	if (pos != std::string::npos)
	{
		pos += search.length();

		size_t	end = str.find('\'', pos);

		if (end != std::string::npos)
			return (str.substr(pos, end - pos));
	}
	return ("");
}

// Extract an unquoted value like key=value
std::string	Request::extractUnquotedValue(const std::string& str, const std::string& key) const
{
	std::string	search = key + "=";
	size_t		pos = str.find(search);

	if (pos != std::string::npos)
	{
		pos += search.length();
		// Skip if it's actually quoted
		if (pos < str.length() && (str[pos] == '"' || str[pos] == '\''))
			return ("");

		size_t	end = str.find_first_of("; \t\r\n", pos);

		if (end == std::string::npos)
			return str.substr(pos);
		return (str.substr(pos, end - pos));
	}
	return ("");
}

bool	Request::isMultipart() const
{
	std::string	ct = getHeader("Content-Type");

	return (ct.find("multipart/form-data") != std::string::npos);
}

std::string	Request::getBoundary() const
{
	std::string	ct = getHeader("Content-Type");
	size_t		pos = ct.find("boundary=");

	if (pos == std::string::npos)
		return ("");

	std::string	boundary = ct.substr(pos + 9);

	// Remove quotes if present
	if (!boundary.empty() && boundary[0] == '"')
	{
		boundary = boundary.substr(1);

		size_t	end = boundary.find('"');

		if (end != std::string::npos)
			boundary = boundary.substr(0, end);
	}

	// Also handle semicolon termination
	size_t	semi = boundary.find(';');

	if (semi != std::string::npos)
		boundary = boundary.substr(0, semi);
	return (trim(boundary));
}

void	Request::parseContentDisposition(const std::string& header, std::string& name, std::string& filename)
{
	name.clear();
	filename.clear();
	
	std::string	trimmed = trim(header);
	
	// Try quoted values first (most common)
	name = extractQuotedValue(trimmed, "name");
	if (name.empty())
		name = extractUnquotedValue(trimmed, "name");	// Try unquoted
	
	filename = extractQuotedValue(trimmed, "filename");
	if (filename.empty())
		filename = extractUnquotedValue(trimmed, "filename");	// Try unquoted
	
	// Handle filename*= (RFC 5987 encoding) for international filenames
	// e.g., filename*=UTF-8''%E4%B8%AD%E6%96%87.txt
	if (filename.empty())
	{
		size_t	fn_star_pos = trimmed.find("filename*=");

		if (fn_star_pos != std::string::npos)
		{
			size_t		start = fn_star_pos + 10;
			size_t		end = trimmed.find_first_of("; \t\r\n", start);
			std::string	encoded_fn;

			if (end == std::string::npos)
				encoded_fn = trimmed.substr(start);
			else
				encoded_fn = trimmed.substr(start, end - start);
			
			// Format: charset'language'encoded_value
			size_t	quote1 = encoded_fn.find('\'');
			size_t	quote2 = encoded_fn.find('\'', quote1 + 1);

			if (quote1 != std::string::npos && quote2 != std::string::npos)
			{
				// Skip charset and language, decode the value
				std::string encoded = encoded_fn.substr(quote2 + 1);
				filename = urlDecode(encoded);
			}
		}
	}
	
	// URL decode the filename if it contains encoded characters
	if (!filename.empty() && filename.find('%') != std::string::npos)
		filename = urlDecode(filename);
	
	// Sanitize filename - remove path components for security
	size_t	last_slash = filename.find_last_of("/\\");
	if (last_slash != std::string::npos)
		filename = filename.substr(last_slash + 1);
	
	// Remove null bytes and other dangerous characters
	std::string	safe_filename;

	for (size_t i = 0; i < filename.length(); i++)
	{
		char	c = filename[i];
		// Skip null bytes and control characters
		if (c != '\0' && c != '\r' && c != '\n' && (unsigned char)c >= 32)
		{
			// Replace potentially dangerous characters
			if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
				safe_filename += '_';
			else
				safe_filename += c;
		}
	}
	filename = safe_filename;
}

void	Request::parseContentType(const std::string& header, std::string& mime_type)
{
	mime_type.clear();
	
	std::string	trimmed = trim(header);
	
	// Extract mime type (before any semicolon)
	size_t	semi = trimmed.find(';');
	if (semi != std::string::npos)
		mime_type = trim(trimmed.substr(0, semi));
	else
		mime_type = trimmed;
}

// Helper to find boundary position accounting for binary data
bool	Request::findBoundaryPosition(const std::string& data, const std::string& boundary, size_t start, size_t& pos) const
{
	std::string delimiter = "--" + boundary;

	// Use memmem-like search for binary safety
	pos = data.find(delimiter, start);
	return (pos != std::string::npos);
}

bool	Request::parseMultipart()
{
	if (multipart_parsed)
		return !multipart_parts.empty();
	multipart_parsed = true;
	if (!isMultipart())
		return (false);
	
	std::string	boundary = getBoundary();

	if (boundary.empty())
	{
		std::cerr << "No boundary found in multipart request" << std::endl;
		return (false);
	}
	
	std::string	delimiter = "--" + boundary;
	std::string	end_delimiter = "--" + boundary + "--";
	size_t		pos;

	if (!findBoundaryPosition(body, boundary, 0, pos))
	{
		std::cerr << "Initial boundary not found in body" << std::endl;
		std::cerr << "Body preview: [" << body.substr(0, std::min(body.length(), (size_t)200)) << "]" << std::endl;
		return (false);
	}
	
	while (pos != std::string::npos)
	{
		// Move past the boundary
		pos += delimiter.length();
		
		// Check for end delimiter (-- after boundary)
		if (pos + 2 <= body.length() && body[pos] == '-' && body[pos+1] == '-')
			break ;  // End of multipart
		
		// Skip CRLF after boundary (some implementations use just LF)
		if (pos < body.length() && body[pos] == '\r')
			pos++;
		if (pos < body.length() && body[pos] == '\n')
			pos++;
		
		// Find end of part headers (double CRLF)
		size_t	header_end = body.find("\r\n\r\n", pos);

		if (header_end == std::string::npos)
		{
			// Try with just LF (non-standard but some clients do this)
			header_end = body.find("\n\n", pos);
			if (header_end == std::string::npos)
			{
				std::cerr << "Part headers not properly terminated at pos " << pos << std::endl;
				break ;
			}
		}

		// Extract and parse part headers
		std::string		part_headers = body.substr(pos, header_end - pos);
		MultipartPart	part;
		std::string		content_disposition;
		std::string		content_type_header;
		
		// Parse part headers line by line
		std::istringstream	header_stream(part_headers);
		std::string			header_line;

		while (std::getline(header_stream, header_line))
		{
			// Remove trailing \r if present
			while (!header_line.empty() && (header_line[header_line.length()-1] == '\r' || header_line[header_line.length()-1] == '\n'))
				header_line.erase(header_line.length()-1);
			
			if (header_line.empty())
				continue ;
			
			// Case-insensitive header matching
			std::string	lower_line = header_line;

			for (size_t i = 0; i < lower_line.length(); i++)
				lower_line[i] = std::tolower(lower_line[i]);
			
			if (lower_line.find("content-disposition:") == 0)
				content_disposition = header_line.substr(20);
			else if (lower_line.find("content-type:") == 0)
				content_type_header = header_line.substr(13);
			else if (lower_line.find("content-transfer-encoding:") == 0)
				part.content_transfer_encoding = trim(header_line.substr(26));
		}

		// Parse Content-Disposition for name and filename
		parseContentDisposition(content_disposition, part.name, part.filename);
		part.is_file = !part.filename.empty();

		// Parse Content-Type for mime type
		if (!content_type_header.empty())
			parseContentType(content_type_header, part.content_type);
		else if (part.is_file)
			part.content_type = "application/octet-stream";	// Default content type for files

		// Calculate content start position
		size_t	content_start = header_end + 4; // Skip \r\n\r\n
		if (body.substr(header_end, 2) == "\n\n")
			content_start = header_end + 2; // Skip \n\n for non-standard
		
		// Find next boundary
		size_t	next_boundary;
		if (!findBoundaryPosition(body, boundary, content_start, next_boundary))
		{
			std::cerr << "Next boundary not found after pos " << content_start << std::endl;
			break ;
		}

		// Content ends before \r\n--boundary (or \n--boundary)
		size_t content_end = next_boundary;

		// Remove trailing CRLF that precedes the boundary
		if (content_end >= 2 && body[content_end - 2] == '\r' && body[content_end - 1] == '\n')
			content_end -= 2;
		else if (content_end >= 1 && body[content_end - 1] == '\n')
			content_end -= 1;

		// Extract raw content
		part.data = body.substr(content_start, content_end - content_start);

		// Handle Content-Transfer-Encoding
		std::string	encoding = part.content_transfer_encoding;
		for (size_t i = 0; i < encoding.length(); i++)
			encoding[i] = std::tolower(encoding[i]);

		if (encoding == "base64")
		{
			// Remove whitespace from base64 data
			std::string	clean_b64;
			for (size_t i = 0; i < part.data.length(); i++)
			{
				char	c = part.data[i];

				if (!isspace(c))
					clean_b64 += c;
			}
			part.data = base64Decode(clean_b64);
		}
		else if (encoding == "quoted-printable")
			part.data = quotedPrintableDecode(part.data);
		// For "binary", "7bit", "8bit", or empty - data is used as-is

		multipart_parts.push_back(part);

		// Move to next part
		pos = next_boundary;
	}
	return (!multipart_parts.empty());
}


