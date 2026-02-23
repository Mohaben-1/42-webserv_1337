#include "../includes/Request.hpp"
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>

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
				i += 2;
				continue ;
			}
			else if (str[i+1] == '\n')
			{
				i += 1;
				continue ;
			}
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
		size_t	line_end = chunked_body.find("\r\n", pos);

		if (line_end == std::string::npos)
			break ;

		std::string	size_str = chunked_body.substr(pos, line_end - pos);
		size_t		semi = size_str.find(';');

		if (semi != std::string::npos)
			size_str = size_str.substr(0, semi);

		char*	end_ptr;
		size_t	chunk_size = strtol(size_str.c_str(), &end_ptr, 16);
		
		if (chunk_size == 0)
			break;

		pos = line_end + 2;

		if (pos + chunk_size <= chunked_body.length())
		{
			result += chunked_body.substr(pos, chunk_size);
			pos += chunk_size;
		}

		if (pos + 2 <= chunked_body.length() && chunked_body[pos] == '\r' && chunked_body[pos + 1] == '\n')
			pos += 2;
	}
	return (result);
}

bool	Request::validateRequestLine()
{
	if (method.empty() || path.empty() || version.empty())
	{
		parse_error = true;
		error_code = 400;
		return (false);
	}

	for (size_t i = 0; i < method.length(); i++)
	{
		if (method[i] < 'A' || method[i] > 'Z')
		{
			parse_error = true;
			error_code = 400;
			return (false);
		}
	}

	if (path[0] != '/' && path != "*")
	{
		parse_error = true;
		error_code = 400;
		return (false);
	}

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

	if (headers_complete)
	{
		size_t	header_end = raw_data.find("\r\n\r\n");

		if (header_end != std::string::npos)
		{
			std::string	raw_body = raw_data.substr(header_end + 4);

			if (is_chunked)
			{
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

	size_t	header_end = raw_data.find("\r\n\r\n");
	if (header_end == std::string::npos)
		return (false);

	std::string			header_section = raw_data.substr(0, header_end);
	std::istringstream	stream(header_section);
	std::string			line;

	if (std::getline(stream, line))
	{
		if (!line.empty() && line[line.length()-1] == '\r')
			line.erase(line.length()-1);

		std::istringstream	request_line(line);
		request_line >> method >> path >> version;
	}

	if (!validateRequestLine())
	{
		headers_complete = true;
		body_complete = true;
		return (true);
	}

	while (std::getline(stream, line))
	{
		if (!line.empty() && line[line.length()-1] == '\r')
			line.erase(line.length()-1);
		if (line.empty())
			break ;

		size_t	colon = line.find(':');
		if (colon != std::string::npos)
		{
			std::string	key = line.substr(0, colon);
			std::string	value = line.substr(colon + 1);

			while (!value.empty() && value[0] == ' ')
				value.erase(0, 1);
			headers[key] = value;
		}
	}

	std::string	cl = getHeader("Content-Length");
	if (!cl.empty())
		content_length = std::atol(cl.c_str());

	if (version == "HTTP/1.1" && getHeader("Host").empty())
	{
		parse_error = true;
		error_code = 400;
		headers_complete = true;
		body_complete = true;
		return (true);
	}

	std::string te = getHeader("Transfer-Encoding");
	if (te.find("chunked") != std::string::npos)
		is_chunked = true;
	headers_complete = true;
	
	body = raw_data.substr(header_end + 4);

	if (is_chunked)
	{
		// For chunked encoding, check for terminating chunk (0\r\n\r\n)
		if (body.find("0\r\n\r\n") != std::string::npos)
		{
			body = unchunkBody(body);
			body_complete = true;
		}
	}
	else if (content_length == 0 || body.length() >= content_length)
	{
		body_complete = true;
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

std::string	Request::extractQuotedValue(const std::string& str, const std::string& key) const
{
	std::string	search = key + "=\"";
	size_t		pos = str.find(search);

	if (pos != std::string::npos)
	{
		pos += search.length();

		size_t	end = str.find('"', pos);

		if (end != std::string::npos)
			return str.substr(pos, end - pos);
	}
	
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

std::string	Request::extractUnquotedValue(const std::string& str, const std::string& key) const
{
	std::string	search = key + "=";
	size_t		pos = str.find(search);

	if (pos != std::string::npos)
	{
		pos += search.length();
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

	if (!boundary.empty() && boundary[0] == '"')
	{
		boundary = boundary.substr(1);

		size_t	end = boundary.find('"');

		if (end != std::string::npos)
			boundary = boundary.substr(0, end);
	}

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
	
	name = extractQuotedValue(trimmed, "name");
	if (name.empty())
		name = extractUnquotedValue(trimmed, "name");
	
	filename = extractQuotedValue(trimmed, "filename");
	if (filename.empty())
		filename = extractUnquotedValue(trimmed, "filename");
	
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
			
			size_t	quote1 = encoded_fn.find('\'');
			size_t	quote2 = encoded_fn.find('\'', quote1 + 1);

			if (quote1 != std::string::npos && quote2 != std::string::npos)
			{
				std::string encoded = encoded_fn.substr(quote2 + 1);
				filename = urlDecode(encoded);
			}
		}
	}

	if (!filename.empty() && filename.find('%') != std::string::npos)
		filename = urlDecode(filename);

	size_t	last_slash = filename.find_last_of("/\\");
	if (last_slash != std::string::npos)
		filename = filename.substr(last_slash + 1);
	
	std::string	safe_filename;

	for (size_t i = 0; i < filename.length(); i++)
	{
		char	c = filename[i];
		if (c != '\0' && c != '\r' && c != '\n' && (unsigned char)c >= 32)
		{
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
	
	size_t	semi = trimmed.find(';');
	if (semi != std::string::npos)
		mime_type = trim(trimmed.substr(0, semi));
	else
		mime_type = trimmed;
}

bool	Request::findBoundaryPosition(const std::string& data, const std::string& boundary, size_t start, size_t& pos) const
{
	std::string delimiter = "--" + boundary;

	pos = data.find(delimiter, start);
	return (pos != std::string::npos);
}

bool	Request::parseMultipart()
{
	if (multipart_parsed)
		return (!multipart_parts.empty());
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
		pos += delimiter.length();

		if (pos + 2 <= body.length() && body[pos] == '-' && body[pos+1] == '-')
			break ;
		
		if (pos < body.length() && body[pos] == '\r')
			pos++;
		if (pos < body.length() && body[pos] == '\n')
			pos++;

		size_t	header_end = body.find("\r\n\r\n", pos);

		if (header_end == std::string::npos)
		{
			header_end = body.find("\n\n", pos);
			if (header_end == std::string::npos)
			{
				std::cerr << "Part headers not properly terminated at pos " << pos << std::endl;
				break ;
			}
		}

		std::string		part_headers = body.substr(pos, header_end - pos);
		MultipartPart	part;
		std::string		content_disposition;
		std::string		content_type_header;
		
		std::istringstream	header_stream(part_headers);
		std::string			header_line;

		while (std::getline(header_stream, header_line))
		{
			while (!header_line.empty() && (header_line[header_line.length()-1] == '\r' || header_line[header_line.length()-1] == '\n'))
				header_line.erase(header_line.length()-1);
			
			if (header_line.empty())
				continue ;
			
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

		parseContentDisposition(content_disposition, part.name, part.filename);
		part.is_file = !part.filename.empty();

		if (!content_type_header.empty())
			parseContentType(content_type_header, part.content_type);
		else if (part.is_file)
			part.content_type = "application/octet-stream";

		size_t	content_start = header_end + 4;
		if (body.substr(header_end, 2) == "\n\n")
			content_start = header_end + 2;
		
		size_t	next_boundary;
		if (!findBoundaryPosition(body, boundary, content_start, next_boundary))
		{
			std::cerr << "Next boundary not found after pos " << content_start << std::endl;
			break ;
		}

		size_t content_end = next_boundary;

		if (content_end >= 2 && body[content_end - 2] == '\r' && body[content_end - 1] == '\n')
			content_end -= 2;
		else if (content_end >= 1 && body[content_end - 1] == '\n')
			content_end -= 1;

		part.data = body.substr(content_start, content_end - content_start);

		std::string	encoding = part.content_transfer_encoding;
		for (size_t i = 0; i < encoding.length(); i++)
			encoding[i] = std::tolower(encoding[i]);

		if (encoding == "base64")
		{
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

		multipart_parts.push_back(part);

		pos = next_boundary;
	}
	return (!multipart_parts.empty());
}
