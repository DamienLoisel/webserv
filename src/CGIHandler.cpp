#include "CGIHandler.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <cerrno>
#include <string>
#include <fcntl.h>

CGIHandler::CGIHandler(const std::string& scriptPath) : _scriptPath(scriptPath) {}

void CGIHandler::setupEnvironment(const std::string& method, const std::string& queryString, const std::string& contentType)
{
    _env["REQUEST_METHOD"] = method;
    _env["QUERY_STRING"] = queryString;
    _env["CONTENT_TYPE"] = contentType;
    std::stringstream ss;
    ss << _requestBody.length();
    _env["CONTENT_LENGTH"] = ss.str();
    
    // Variables d'environnement requises par CGI
    _env["SCRIPT_FILENAME"] = _scriptPath;
    _env["SCRIPT_NAME"] = "/cgi-bin/post.py";  // Le chemin relatif au document root
    _env["PATH_INFO"] = "";  // Pas de path info dans notre cas
    _env["SERVER_NAME"] = "127.0.0.1";
    _env["SERVER_PORT"] = "7000";
    _env["SERVER_SOFTWARE"] = "webserv/1.0";
    _env["SERVER_PROTOCOL"] = "HTTP/1.1";
    _env["GATEWAY_INTERFACE"] = "CGI/1.1";
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

std::string CGIHandler::executeCGI(const std::string& method, const std::string& queryString, const std::string& requestBody, const std::string& contentType)
{
    _requestBody = requestBody;
    setupEnvironment(method, queryString, contentType);

    int inputPipe[2];
    int outputPipe[2];
    int errorPipe[2];
    
    if (pipe(inputPipe) < 0 || pipe(outputPipe) < 0 || pipe(errorPipe) < 0) {
        throw std::runtime_error("Erreur création pipe");
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(inputPipe[0]);
        close(inputPipe[1]);
        close(outputPipe[0]);
        close(outputPipe[1]);
        close(errorPipe[0]);
        close(errorPipe[1]);
        throw std::runtime_error("Erreur fork");
    }

    if (pid == 0) {
        // Changer vers le répertoire du script
        std::string scriptDir = _scriptPath.substr(0, _scriptPath.find_last_of('/'));
        std::string scriptName = _scriptPath.substr(_scriptPath.find_last_of('/') + 1);
        
        if (chdir(scriptDir.c_str()) != 0) {
            _exit(1);
        }

        close(inputPipe[1]);
        close(outputPipe[0]);
        close(errorPipe[0]);

        if (dup2(inputPipe[0], STDIN_FILENO) == -1) {
            _exit(1);
        }
        if (dup2(outputPipe[1], STDOUT_FILENO) == -1) {
            _exit(1);
        }
        if (dup2(errorPipe[1], STDERR_FILENO) == -1) {
            _exit(1);
        }

        close(inputPipe[0]);
        close(outputPipe[1]);
        close(errorPipe[1]);

        // Définir une alarme de 5 secondes
        alarm(5);
        
        char** env = createEnvArray();
        
        // Déterminer l'interpréteur basé sur l'extension
        std::string interpreter;
        size_t dot_pos = scriptName.find_last_of('.');
        if (dot_pos != std::string::npos) {
            std::string ext = scriptName.substr(dot_pos);
            if (ext == ".py") {
                interpreter = "/usr/bin/python3";
            } else if (ext == ".sh") {
                interpreter = "/bin/bash";
            } else {
                _exit(1);
            }
        } else {
            _exit(1);
        }
        
        char* args[] = { 
            const_cast<char*>(interpreter.c_str()),
            const_cast<char*>(scriptName.c_str()),
            NULL 
        };
        
        execve(interpreter.c_str(), args, env);
        _exit(1);
    }

    // Parent process
    close(inputPipe[0]);
    close(outputPipe[1]);
    close(errorPipe[1]);

    // Écrire les données POST
    if (!_requestBody.empty()) {
        size_t totalWritten = 0;
        const char* data = _requestBody.c_str();
        size_t remaining = _requestBody.length();
        
        while (remaining > 0) {
            ssize_t written = write(inputPipe[1], data + totalWritten, remaining);
            if (written <= 0) {
                close(inputPipe[1]);
                close(outputPipe[0]);
                close(errorPipe[0]);
                throw std::runtime_error("Erreur écriture données POST");
            }
            totalWritten += written;
            remaining -= written;
        }
    }
    close(inputPipe[1]);

    std::string response;
    std::string error_output;
    char buffer[4096];
    ssize_t bytesRead;
    
    // Lire stderr
    while ((bytesRead = read(errorPipe[0], buffer, sizeof(buffer))) > 0) {
        error_output.append(buffer, bytesRead);
    }
    close(errorPipe[0]);
    
    // Lire stdout
    while ((bytesRead = read(outputPipe[0], buffer, sizeof(buffer))) > 0) {
        response.append(buffer, bytesRead);
    }
    
    if (bytesRead < 0) {
        close(outputPipe[0]);
        throw std::runtime_error("Erreur lecture réponse CGI");
    }
    close(outputPipe[0]);

    int status;
    waitpid(pid, &status, 0);
    
    if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status != 0) {
            std::stringstream ss;
            ss << "CGI script failed with status " << exit_status;
            if (!error_output.empty()) {
                ss << "\nError output:\n" << error_output;
            }
            throw std::runtime_error(ss.str());
        }
    } else if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        if (sig == SIGALRM) {
            throw std::runtime_error("CGI script timeout after 5 seconds");
        }
        throw std::runtime_error("CGI script terminated by signal");
    }

    if (response.empty()) {
        throw std::runtime_error("CGI script returned empty response");
    }

    return response;
}