#include "CGIHandler.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <stdexcept>
#include <sstream>

CGIHandler::CGIHandler(const std::string& scriptPath) : _scriptPath(scriptPath) {}

void CGIHandler::setupEnvironment(const std::string& method, const std::string& queryString)
{
    _env["REQUEST_METHOD"] = method;
    _env["QUERY_STRING"] = queryString;
    std::stringstream ss;
    ss << _requestBody.length();
    _env["CONTENT_LENGTH"] = ss.str();
    _env["SCRIPT_FILENAME"] = _scriptPath;
    _env["REDIRECT_STATUS"] = "200";
}

char** CGIHandler::createEnvArray() {
    std::vector<std::string> envStrings;
    for (std::map<std::string, std::string>::const_iterator it = _env.begin(); 
         it != _env.end(); ++it) {
        envStrings.push_back(it->first + "=" + it->second);
    }
    
    char** env = new char*[envStrings.size() + 1];
    for (size_t i = 0; i < envStrings.size(); i++) {
        env[i] = strdup(envStrings[i].c_str());
    }
    env[envStrings.size()] = NULL;
    return env;
}

void CGIHandler::cleanupEnvArray(char** env) {
    if (!env) return;
    for (int i = 0; env[i] != NULL; i++) {
        delete(env[i]);
    }
    delete[] env;
}

std::string CGIHandler::executeCGI(const std::string& method, const std::string& queryString, const std::string& requestBody)
{
    _requestBody = requestBody;
    setupEnvironment(method, queryString);

    int inputPipe[2];
    int outputPipe[2];
    
    if (pipe(inputPipe) < 0 || pipe(outputPipe) < 0)
    {
        throw std::runtime_error("Erreur création pipe");
    }

    pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("Erreur fork");
    }

    if (pid == 0) 
    {
        close(inputPipe[1]);
        close(outputPipe[0]);

        dup2(inputPipe[0], STDIN_FILENO);
        dup2(outputPipe[1], STDOUT_FILENO);

        char** env = createEnvArray();
        char* args[] = { const_cast<char*>(_scriptPath.c_str()), NULL };
        execve(_scriptPath.c_str(), args, env);
        cleanupEnvArray(env);
        _exit(1);
    }

    close(inputPipe[0]);
    close(outputPipe[1]);

    if (!_requestBody.empty())
        write(inputPipe[1], _requestBody.c_str(), _requestBody.length());
    close(inputPipe[1]);

    std::string response;
    char buffer[4096];
    ssize_t bytesRead;
    
    while ((bytesRead = read(outputPipe[0], buffer, sizeof(buffer))) > 0)
        response.append(buffer, bytesRead);

    if (response.find("HTTP/1.") != 0) {

        std::string headers = "HTTP/1.1 200 OK\r\n";
        headers += "Content-Type: text/html\r\n";
        
        std::stringstream ss;
        ss << response.length();
        headers += "Content-Length: " + ss.str() + "\r\n";
        headers += "\r\n";
        
        response = headers + response;
    }
    
    close(outputPipe[0]);

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        throw std::runtime_error("Erreur execution CGI");

    return response;
}