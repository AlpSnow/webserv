/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cnamoune <cnamoune@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 17:40:46 by mlavry            #+#    #+#             */
/*   Updated: 2026/06/18 17:17:36 by cnamoune         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"
#include "CgiHandler.hpp"
#include "Client.hpp"
#include <sys/stat.h>
#include <iostream>
#include <algorithm>

HttpResponse::HttpResponse()
{
    status_code = 200;
    bytes_sent = 0;
    state = RESPONSE_BUILDING;
    server_config = NULL;
	location_config = NULL;
};

HttpResponse::~HttpResponse()
{
    
};

bool	HttpResponse::build_error_from_error_page(int error_code, const ServerConfig *config, const Request& request)
{
		std::string active_root = config->root;
        if (!active_root.empty() && active_root[active_root.length() - 1] != '/')
            active_root += "/";
            
        std::string full_error_path = active_root + config->errorPages.at(error_code);

        std::ifstream file(full_error_path.c_str(), std::ios::binary);
        if (file.is_open())
        {
            body.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
            file.close();

            assemble_response(request);
            return (true);
        }

	return (false);
}

void	HttpResponse::generate_error(int error_code, const ServerConfig *config, const Request& request)
{
	std::stringstream	error_message;
	
	state = RESPONSE_BUILDING;
	server_config = config;
	status_code = error_code;
	headers["Content-Type"] = "text/html";
	
	if (config && config->errorPages.find(status_code) != config->errorPages.end())
	{
        if (build_error_from_error_page(status_code, config, request))
            return ;
	}

	error_message   << "<html><body><h1>Error " << status_code << std::endl
                    << " " << get_status_message(status_code)
                    << "</h1></body></html>";
	std::string	str = error_message.str();
	body.assign(str.begin(), str.end());
	

	assemble_response(request);
}

void	HttpResponse::assemble_response(const Request& request)
{
	std::stringstream	header_steam;

	header_steam << "HTTP/1.1 " << status_code << " " << get_status_message(status_code)
				<< "\r\n";
	if (!body.empty())
	{
		std::stringstream	content_size;
		content_size << body.size();
		headers["Content-Length"] = content_size.str();
	}
	else
		headers["Content-Length"] = "0";

    std::map<std::string, std::string>::const_iterator	const_it = request.header.find("Connection");
	if (const_it == request.header.end())
		headers["Connection"] = "keep-alive";
    else if (!const_it->second.empty() && const_it->second == "close")
    {
        headers["Connection"] = "close";
    }
    else
    {
        headers["Connection"] = "keep-alive";
    }

	std::map<std::string, std::string>::const_iterator	it;
	for (it = headers.begin(); it != headers.end(); ++it)
		header_steam << it->first << ": " << it->second << "\r\n";
	header_steam << "\r\n";
	
	std::string	header_str = header_steam.str();
	raw_response.clear();
	raw_response.insert(raw_response.end(), header_str.begin(), header_str.end());
	if (request.method != "HEAD")
		raw_response.insert(raw_response.end(), body.begin(), body.end());
	
	bytes_sent = 0;
	state = RESPONSE_READY_TO_SEND;
}

void HttpResponse::build_autoindex(const std::string& directory_path, const Request& request)
{
    std::stringstream html;

    html << "<html>\n<head><title>Index of " << directory_path << "</title></head>\n<body>\n";
    html << "<link rel=\"stylesheet\" href=\"/theme.css\">\n";
    html << "<h1>Index of " << directory_path << "</h1>\n<hr>\n<pre>\n";
    
    DIR* directory = opendir(directory_path.c_str());
    if (!directory)
    {
        if (errno == EACCES)
            generate_error(403, server_config, request);
        else
        {
            generate_error(500, server_config, request);   
        }
        return ;
    }

    struct dirent* ent;
    while ((ent = readdir(directory)) != NULL)
    {
        std::string file_name = ent->d_name;
        html << "<a href=\"" << file_name << "\">" << file_name << "</a>\n";
    }

    closedir(directory);

    html << "</pre>\n<hr>\n";

    if (this->location_config && this->location_config->uploadEnabled)
    {
        html << "<h3>Upload to this directory:</h3>\n";
        html << "<form action=\"\" method=\"POST\" enctype=\"multipart/form-data\">\n";
        html << "  <input type=\"file\" name=\"file\" required>\n";
        html << "  <input type=\"submit\" value=\"Upload File\" style=\"margin-top: 10px; padding: 5px 10px;\">\n";
        html << "</form>\n<hr>\n";
    }

    html << "</body>\n</html>\n";

    std::string succes_upload_body = html.str();
    body.assign(succes_upload_body.begin(), succes_upload_body.end());

    headers["Content-Type"] = "text/html";
    // status_code = 200;
    
    assemble_response(request);
}

bool	HttpResponse::extract_filename(const std::vector<char>& body, std::string& out_filename, size_t& file_start, size_t& file_length)
{
    const char* filename_tag = "filename=\"";

    std::vector<char>::const_iterator it = std::search(body.begin(), body.end(), filename_tag, filename_tag + 10);
    if (it != body.end())
    {
        std::vector<char>::const_iterator name_start = it + 10;
        std::vector<char>::const_iterator name_end = std::find(name_start, body.end(), '\"');
        if (name_end != body.end())
            out_filename.assign(name_start, name_end);
        else
            return (false);
    }
    else
    {
        return (false);
	}
    
	const char	*header_end = "\r\n\r\n";
	std::vector<char>::const_iterator	file_body_start = std::search(body.begin(), body.end(),
																header_end, header_end + 4);

	if (file_body_start != body.end())
	{
		file_start = (file_body_start - body.begin()) + 4;
	
		const char	*file_delimiter = "\r\n--";
		std::vector<char>::const_iterator file_body_end = std::search(body.begin() + file_start, body.end(), 
            file_delimiter, file_delimiter + 4);
		
		if (file_body_end != body.end())
			file_length = file_body_end - (body.begin() + file_start);
		else
			file_length = body.size() - file_start;
		return (true);
	}
	return (false);
}

bool	HttpResponse::body_size_to_big(const Request& request)
{
	size_t max_size = 100000001;

    // if (location_config->hasRedirect)
    // {
    //     std::cerr << "Redir on" << std::endl;
    // }
	if (location_config && location_config->hasClientMaxBodySize)
		max_size = location_config->clientMaxBodySize;
	if (request.body.size() > max_size)
	{
		// std::cerr << "ERROR FROM request.body.size() > max_size" << std::endl;
        this->status_code = 413;
		return (true);
	}
	if (request.header.find("Content-Length") != request.header.end())
	{
		size_t content_length = std::atoi(request.header.at("Content-Length").c_str());
        // std::cerr << content_length << std::endl;
        // std::cerr << max_size << std::endl;
		if (content_length != request.body.size())
		{
            // std::cerr << "ERROR FROM content_length != max_size" << std::endl;
			this->status_code = 400;
			return (true);
		}
	}
	return (false);
}

void	HttpResponse::handle_post(const Request& request)
{
	if (!this->location_config)
    {
		return (generate_error(403, server_config, request));
    }
	if (body_size_to_big(request))
	{
        return (generate_error(this->status_code, server_config, request));
	}
	if (this->location_config->uploadStore.empty())
	{
		this->headers["Content-Type"] = "text/plain";
		
		std::string ok_body = "POST body received\n";
		this->body.assign(ok_body.begin(), ok_body.end());
        assemble_response(request);
        return ;
	}

    std::string upload_directory_path = this->location_config->uploadStore;

    if (upload_directory_path[upload_directory_path.length() - 1] != '/')
        upload_directory_path += "/";

    std::string	filename;
    size_t		file_start = 0;
    size_t		file_length = request.body.size();

    std::string content_type;
    if (request.header.find("Content-Type") != request.header.end())
        content_type = request.header.at("Content-Type");

    if (content_type.find("multipart/form-data") != std::string::npos)
    {
        // std::cout << "There is a multipart" << std::endl;
        if (!extract_filename(request.body, filename, file_start, file_length))
        {
            // std::cerr << "400 from no multipart" << std::endl;
            return (generate_error(400, server_config, request));
        }
    }
    else
    {
		size_t last_slash = request.path.find_last_of('/');
		if (last_slash != std::string::npos && last_slash < request.path.length() - 1)
		{
			filename = request.path.substr(last_slash + 1);
		}
    }

    std::string		final_output_path = upload_directory_path + filename;
    std::ofstream	output_file(final_output_path.c_str(), std::ios::binary);
    
    if (!output_file.is_open())
	{
        // std::cerr << "Error 500 from open file" << std::endl;
		return (generate_error(500, server_config, request));
	}

    output_file.write(&request.body[file_start], file_length);
    output_file.close();
	
	this->status_code = 201;
	this->headers["Content-Type"] = "text/html";
	
	std::stringstream str;
    str << "<html><body><h1>201 Created</h1><p>File " << filename << " uploaded successfully!</p></body></html>";
    std::string succes_upload_body = str.str();
    
    this->body.assign(succes_upload_body.begin(), succes_upload_body.end());
    assemble_response(request);
}

void	HttpResponse::handle_get(const Request& request)
{
    if (this->target_path_info == PATH_IS_DIRECTORY)
    {
        build_autoindex(target_file_path, request);
        return ;
    }
    
    std::ifstream file(target_file_path.c_str(), std::ios::binary);

    if (!file)
    {
        generate_error(403, this->server_config, request);
        return ;
    }

    body.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    file.close();
    headers["Content-Type"] = get_mime_type(target_file_path);

    assemble_response(request);
}

void	HttpResponse::handle_delete(const Request& request)
{
	if (this->target_path_info == PATH_IS_DIRECTORY)
    {
        generate_error(403, server_config, request);
        return ;
    }

    if (std::remove(target_file_path.c_str()) != 0)
    {
        generate_error(403, server_config, request); 
        return ;
    }
    this->status_code = 204;
    this->body.clear();

    assemble_response(request);
}

void	HttpResponse::handle_head(const Request& request)
{
    if (this->target_path_info == PATH_IS_DIRECTORY)
    {
        headers["Content-Type"] = "text/html";

        assemble_response(request);
        return;
    }

    struct stat buffer;
    if (stat(target_file_path.c_str(), &buffer) != 0)
    {
        generate_error(500, server_config, request); 
        return ;
    }

    headers["Content-Type"] = get_mime_type(target_file_path);

    std::stringstream size_stream;
    size_stream << buffer.st_size;
    headers["Content-Length"] = size_stream.str();

    this->body.clear();
    assemble_response(request);
}

void HttpResponse::handle_cgi(const Request& request, const ServerConfig *config, Client& client,
                                std::string cgi_extention, const std::string& target_path_file, PathInfo path_info)
{
    this->status_code = client.cgi.handle_script(request, cgi_extention, executable, target_path_file, path_info);
    if (client.cgi.stats == CGI_ERROR)
        generate_error(this->status_code, config, request);
    return ;
}

void HttpResponse::print_cgi_output(const std::vector<char>& cgi_output) const
{
    std::cout << "CGI output:\n";
    std::cout.write(&cgi_output[0], cgi_output.size());
    std::cout << std::endl;
}

void    HttpResponse::parse_cgi_output(const std::vector<char>& cgi_outpout, const Request& request)
{
    // print_cgi_output(cgi_outpout);

    std::string delimiter = "\r\n\r\n";

    std::vector<char>::const_iterator it = cgi_outpout.begin();
    std::vector<char>::const_iterator end = cgi_outpout.end();
    std::vector<char>::const_iterator header_end = std::search(it, end, delimiter.begin(), delimiter.end());

    if (header_end != end)
    {
        std::string         header_str(it, header_end);
        std::istringstream  header_stream(header_str);
        std::string         line;

        while (std::getline(header_stream, line))
        {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos)
            {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);

                value.erase(0, value.find_first_not_of(" \t\r\n"));
                value.erase(value.find_last_not_of(" \t\r\n") + 1);

                if (key == "Status" || key == "status")
                    this->status_code = std::atoi(value.c_str());
                else
                    headers[key] = value;
            }
        }
        body.assign(header_end + 4, end);
    }
    else
    {
        // std::cerr << "ERROR From the cgi output" << std::endl;
        generate_error(500, server_config, request);
        return ;
    }
    assemble_response(request);
}

void 	HttpResponse::build_theme_cookie_response(const Request& request, 
			const std::string& theme)
{
	status_code = 302;
	headers["Location"] = "/";
	headers["Set-Cookie"] = "theme=" + theme + "; Path=/; Max-Age=3600";
	body.clear();
	assemble_response(request);
}

void	HttpResponse::build_theme_session_response(const Request& request,
	SessionManager& sessionManager, const std::string& theme)
{
	std::map<std::string, std::string>::const_iterator	it;
	std::string											session_id;
	bool												new_session;

	new_session = false;
	it = request.cookies.find("SESSION_ID");
	if (it != request.cookies.end()
		&& sessionManager.hasSession(it->second))
	{
		session_id = it->second;
	}
	else
	{
		session_id = sessionManager.generateSession();
		new_session = true;
	}
	Session& session = sessionManager.getSession(session_id);
	session.data["theme"] = theme;
	status_code = 302;
	headers["Location"] = "/";
	if (new_session)
		headers["Set-Cookie"] = "SESSION_ID=" + session_id
			+ "; Path=/; HttpOnly; Max-Age=3600";
	body.clear();
	assemble_response(request);
}

static std::string	build_session_html(const std::string& session_id,
	bool new_session)
{
	std::stringstream	html;

	html << "<html>\n";
	html << "<head>\n";
	html << "<title>Session demo</title>\n";
	html << "<link rel=\"stylesheet\" href=\"/theme.css\">\n";
	html << "</head>\n";
	html << "<body>\n";
	html << "<h1>Session demo</h1>\n";
	if (new_session)
		html << "<p>New session created</p>\n";
	else
		html << "<p>Existing session found</p>\n";
	html << "<p>Session ID: " << session_id << "</p>\n";
	html << "<p><a href=\"/\">Back to home</a></p>\n";
	html << "</body>\n";
	html << "</html>\n";
	return (html.str());
}

void	HttpResponse::build_session_response(const Request& request,
	SessionManager& sessionManager)
{
	std::map<std::string, std::string>::const_iterator	it;
	std::string											session_id;
	std::string											html;
	bool												new_session;

	new_session = false;
	it = request.cookies.find("SESSION_ID");
	if (it != request.cookies.end()
		&& sessionManager.hasSession(it->second))
	{
		session_id = it->second;
	}
	else
	{
		session_id = sessionManager.generateSession();
		new_session = true;
	}
	Session& session = sessionManager.getSession(session_id);
	status_code = 200;
	headers["Content-Type"] = "text/html";
	if (new_session)
		headers["Set-Cookie"] = "SESSION_ID=" + session_id
			+ "; Path=/; HttpOnly; Max-Age=3600";
	html = build_session_html(session.id, new_session);
	body.clear();
	body.assign(html.begin(), html.end());
	assemble_response(request);
}

void	HttpResponse::build_theme_css_response(const Request& request,
	SessionManager& sessionManager)
{
	std::map<std::string, std::string>::const_iterator	it;
	std::string											css;
	std::string											theme;

	theme = "light";
	it = request.cookies.find("SESSION_ID");
	if (it != request.cookies.end()
		&& sessionManager.hasSession(it->second))
	{
		Session& session = sessionManager.getSession(it->second);
		if (session.data.find("theme") != session.data.end())
			theme = session.data["theme"];
	}
	if (theme == "dark")
	{
		css = "body { background: #121212; color: #eaeaea; }\n";
		css += ".box { background: #1f1f1f; color: #eaeaea; }\n";
		css += "h1, h2 { color: #ffffff; }\n";
		css += "h3 { color: #cccccc; }\n";
		css += "a { color: #66aaff; }\n";
		css += "code { background: #333333; color: #eaeaea; }\n";
		css += "input { background: #2a2a2a; color: white; border: 1px solid #555; }\n";
	}
	else
	{
		css = "body { background: white; color: black; }\n";
		css += ".box { background: white; color: black; }\n";
		css += "a { color: #0066cc; }\n";
	}
	status_code = 200;
	headers["Content-Type"] = "text/css";
	body.clear();
	body.assign(css.begin(), css.end());
	assemble_response(request);
}

void    HttpResponse::generate(Request& request, const ServerConfig* config, Client& client, SessionManager& sessionManager)
{
	reset();

    this->server_config = config;

    if (!this->server_config)
	{
        // std::cerr << "Error 500 from server config" << std::endl;
		return (generate_error(500, server_config, request));
	}

    state = RESPONSE_BUILDING;

	if (request.method == "GET" && request.path == "/set-theme-dark")
	{
		//build_theme_cookie_response(request, "dark", sessionManager);
		build_theme_session_response(request, sessionManager, "dark");
		return ;
	}
	if (request.method == "GET" && request.path == "/set-theme-light")
	{
		build_theme_session_response(request, sessionManager, "light");
		//build_theme_cookie_response(request, "light", sessionManager);
		return ;
	}
	if (request.method == "GET" && request.path == "/session")
	{
		build_session_response(request, sessionManager);
		return ;
	}
	if (request.method == "GET" && request.path == "/theme.css")
	{
		//build_theme_cookie_response(request, "dark", sessionManager);
		build_theme_css_response(request, sessionManager);
		return ;
	}
    
	translate_path(request);
    // std::cerr << this->target_file_path << "Is the target file path AFTER translate path" << std::endl;
    // std::cerr << this->target_path_info << std::endl;
	std::string cgi_extention = is_cgi_requested(this->target_file_path);

    if (cgi_extention != "NO" && this->status_code == 404)
	{
		// std::cerr << "Statue code maj a cgi" << std::endl;
        this->status_code = 200;
	}
    // std::cerr << this->status_code << "Is the status code before check 200" << std::endl;
	if (status_code != 200 && !(status_code >= 300 && status_code < 400))
	{
        generate_error(this->status_code, config, request);
        return ;
	}
    
	if (status_code >= 300 && status_code < 400)
	{
		// std::cerr << "Redir detected" << std::endl;
		this->headers["Location"] = location_config->redirectTarget;
		std::string redirect_html = "<html><body><h1>301 Moved Permanently</h1><p>The document has moved <a href=\"" + location_config->redirectTarget + "\">here</a>.</p></body></html>";
		this->body.assign(redirect_html.begin(), redirect_html.end());
		assemble_response(request);
		return ;
	}

	if (this->location_config && this->location_config->hasMethods)
	{
		if (this->location_config->methods.count(request.method) == 0)
            return (generate_error(405, config, request));
	}

    if (cgi_extention != "NO" && (request.method == "POST" || request.method == "GET"))
    {
        handle_cgi(request, config, client, cgi_extention, this->target_file_path, this->target_path_info);
    }
	else if (request.method == "GET")
        handle_get(request);
    else if (request.method == "POST")
        handle_post(request);
    else if (request.method == "DELETE")
        handle_delete(request);
    else if (request.method == "HEAD")
        handle_head(request);
}

std::string	HttpResponse::is_cgi_requested(const std::string& target_path)
{
    size_t extention_pos = target_path.find_last_of(".");

    if (extention_pos != std::string::npos)
    {
        std::string cgi_extention = target_path.substr(extention_pos, target_path.length());
        if (!this->location_config)
        {
            // std::cerr << "location config NULL !" << std::endl;
            return ("NO");
        }

        for (std::map<std::string, std::string>::const_iterator it = location_config->cgi.begin();
                 it != location_config->cgi.end(); ++it)
        {
            if (it->first == cgi_extention && !it->second.empty())
            {
                this->executable = it->second;
				return (cgi_extention);
            }
        }
    }

    return ("NO");  
}

std::string	HttpResponse::get_status_message(int code) const
{
	switch (code)
	{
		case 200:	return "OK";
		case 201:	return "Created";
		case 204:	return "No Content";
        case 301:   return "Moved Permanently";
        case 302:   return "Found";
		case 400:	return "Bad Request";
		case 401:	return "Unauthorized";
		case 403:	return "Forbidden";
		case 404:	return "Not Found";
		case 405:	return "Method Not Allowed";
		case 408:	return "Request Timeout";
		case 413:	return "Payload Too Large";
		case 500:	return "Internal Server Error";
		case 501:	return "Not Implemented";
		case 502:	return "Bad Gateway";
		case 503:	return "Service Unavailable";
		case 504:	return "Gateway Timeout";
		case 505:	return "HTTP Version Not Supported";
		default:	return "Error";
	}
}

std::string	HttpResponse::get_mime_type(const std::string& file_path)
{
    size_t dot_pos = file_path.find_last_of('.');
    if (dot_pos == std::string::npos)
        return "application/octet-stream";

    std::string ext = file_path.substr(dot_pos + 1);
    if (ext == "html" || ext == "htm")
        return "text/html";
    if (ext == "css")
        return "text/css";
    if (ext == "js")
        return "application/javascript";
    if (ext == "json")
        return "application/json";
    if (ext == "txt")
        return "text/plain";
    if (ext == "jpg" || ext == "jpeg")
        return "image/jpeg";
    if (ext == "png")
        return "image/png";
    if (ext == "gif")
        return "image/gif";
    if (ext == "pdf")
		return "application/pdf";
	if (ext == ".ico")
		return ("image/x-icon");
    return "application/octet-stream";
}

void    HttpResponse::add_bytes_sent(size_t bytes)
{
    bytes_sent += bytes;
    if (bytes_sent >= raw_response.size())
        state = RESPONSE_COMPLETE;
    else
        state = RESPONSE_SENDING;
}

int             HttpResponse::get_http_status() const
{
    return (status_code);
}

ResponseState    HttpResponse::get_state() const
{
    return (state);
}

size_t	HttpResponse::get_remaining_bytes() const
{
	return (raw_response.size() - bytes_sent);
}

const char*	HttpResponse::get_response_data() const
{
	return (raw_response.data() + bytes_sent);
}

std::string HttpResponse::select_active_index() const
{
    std::string active_index = "index.html";
    if (this->location_config && !this->location_config->index.empty())
        active_index = this->location_config->index;
    else if (this->server_config && !this->server_config->index.empty())
        active_index = this->server_config->index;
    return (active_index);
}

void HttpResponse::handle_directory_path(std::string& full_path)
{
    if (full_path.empty() || full_path[full_path.length() - 1] != '/')
        full_path += '/';

    std::string active_index = select_active_index();
    std::string index_path = full_path + active_index;

    PathInfo index_info = get_path_info(index_path);
    if (index_info == PATH_IS_FILE)
    {
        this->target_file_path = index_path;
        this->target_path_info = PATH_IS_FILE;
        return ;
    }
    if (index_info == PATH_PERMISSION_DENIED)
    {
        // std::cerr << "403 FROM HANDLE DIRECTORY PATH" << std::endl;
        this->status_code = 403;
        return ;
    }

    bool autoindex_on = false;
    if (this->location_config && this->location_config->hasAutoindex)
    {
		autoindex_on = this->location_config->autoindex;
	}
	
    if (autoindex_on)
        this->target_file_path = full_path;
    else
    {
        // std::cerr << "404 FROM HANDLE DIRECTORY PATH" << std::endl;
        this->status_code = 404;
    }
}

PathInfo HttpResponse::get_path_info(const std::string& path) const
{
    struct stat buffer;

    // std::cerr << path << std::endl;
    if (stat(path.c_str(), &buffer) != 0)
    {
        // std::cerr << "STAT DID NOT RETURN 0" << std::endl;
        if (errno == EACCES)
		{
            return (PATH_PERMISSION_DENIED);
    	}
    	return (PATH_DOES_NOT_EXIST);
    }
    if (S_ISDIR(buffer.st_mode))
        return (PATH_IS_DIRECTORY);
    return (PATH_IS_FILE);
}

void	HttpResponse::build_final_path(std::string full_path, const Request& request)
{
	this->target_path_info = get_path_info(full_path);
    // std::cerr << this->target_path_info << std::endl;
    this->target_file_path = full_path;

	if (target_path_info == PATH_DOES_NOT_EXIST && request.method != "POST")
	{
		// std::cerr << "ERROR FROM BUILD FINAL PATH" << std::endl;
        this->status_code = 404;
	}
	else if (target_path_info == PATH_PERMISSION_DENIED)
	{
		// std::cerr << "ERROR FROM BUILD FINAL PATH" << std::endl;
        this->status_code = 403;
	}
	else if (target_path_info == PATH_IS_FILE)
	{
		this->target_file_path = full_path;
	}
	else if (target_path_info == PATH_IS_DIRECTORY)
	{
        
		handle_directory_path(full_path);
	}
}

void	HttpResponse::build_url(const Request& request)
{
	std::string	active_root = server_config->root;
	std::string	path_to_join = request.path;
	bool		location_root = false;

	if (this->location_config && !this->location_config->root.empty())
	{
		location_root = true;
		active_root = this->location_config->root;
	}
	
	if (!active_root.empty() && active_root[active_root.length() - 1] == '/')
		active_root.erase(active_root.length() - 1);
	
	if (location_root)
		path_to_join = request.path.substr(this->location_config->path.length());
	
	if (!path_to_join.empty() && path_to_join[0] != '/')
		path_to_join = "/" + path_to_join;

	build_final_path(active_root + path_to_join, request);
}

void	HttpResponse::translate_path(Request& request)
{
    this->location_config = NULL;
    size_t longest_match = 0;

    for (size_t i = 0; i < server_config->locations.size(); ++i)
    {
        std::string loc_path = server_config->locations[i].path;
        
        if (request.path.find(loc_path) == 0)
        {
            if (loc_path == "/" || request.path.length() == loc_path.length() || request.path[loc_path.length()] == '/')
            {
                if (loc_path.length() > longest_match)
                {
                    longest_match = loc_path.length();
                    this->location_config = &server_config->locations[i];
                }
            }
        }
    }

    if (this->location_config && this->location_config->hasRedirect)
    {
        // std::cerr << "redir detected" << std::endl;
        // request.path = this->location_config->redirectTarget;
		// std::cerr << request.path << std::endl;
        this->status_code = this->location_config->redirectCode;
        return ;
    }
    build_url(request);
}

void HttpResponse::reset()
{
    status_code = 200;
    bytes_sent = 0;
    state = RESPONSE_BUILDING;
    location_config = NULL;
    target_file_path.clear();
    executable.clear();
    body.clear();
    raw_response.clear();
    headers.clear();
}