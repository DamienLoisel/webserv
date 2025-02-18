#include "CGIHandler.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <stdexcept>

// Constructeur
CGIHandler::CGIHandler(const std::string& scriptPath) : _scriptPath(scriptPath) {}

// Configure les variables d'environnement pour le CGI
void CGIHandler::setupEnvironment(const std::string& method, const std::string& queryString)
{
    _env["REQUEST_METHOD"] = method;
    _env["QUERY_STRING"] = queryString;
    _env["CONTENT_LENGTH"] = std::to_string(_requestBody.length());
    _env["SCRIPT_FILENAME"] = _scriptPath;
    _env["REDIRECT_STATUS"] = "200";
}

// Crée un tableau d'environnement pour execve
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

// Exécute le script CGI
std::string CGIHandler::executeCGI(const std::string& method, 
                                  const std::string& queryString, 
                                  const std::string& requestBody) {
    _requestBody = requestBody;
    setupEnvironment(method, queryString);

    int inputPipe[2];  // Pour envoyer des données au CGI
    int outputPipe[2]; // Pour recevoir des données du CGI
    
    if (pipe(inputPipe) < 0 || pipe(outputPipe) < 0) {
        throw std::runtime_error("Erreur création pipe");
    }

    pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("Erreur fork");
    }

    if (pid == 0) { // Processus enfant
        close(inputPipe[1]);  // Ferme l'extrémité d'écriture du pipe d'entrée
        close(outputPipe[0]); // Ferme l'extrémité de lecture du pipe de sortie

        // Redirige stdin vers le pipe d'entrée
        dup2(inputPipe[0], STDIN_FILENO);
        // Redirige stdout vers le pipe de sortie
        dup2(outputPipe[1], STDOUT_FILENO);

        char** env = createEnvArray();
        char* args[] = {
            const_cast<char*>(_scriptPath.c_str()),
            NULL
        };

        // Change le répertoire de travail si nécessaire
        // chdir("/path/to/cgi/directory");

        execve(_scriptPath.c_str(), args, env);
        // Si on arrive ici, c'est qu'il y a eu une erreur
        exit(1);
    }

    // Processus parent
    close(inputPipe[0]);  // Ferme l'extrémité de lecture du pipe d'entrée
    close(outputPipe[1]); // Ferme l'extrémité d'écriture du pipe de sortie

    // Envoie le corps de la requête au CGI
    if (!_requestBody.empty()) {
        write(inputPipe[1], _requestBody.c_str(), _requestBody.length());
    }
    close(inputPipe[1]); // Signale EOF au CGI

    // Lit la réponse du CGI
    std::string response;
    char buffer[4096];
    ssize_t bytesRead;
    
    while ((bytesRead = read(outputPipe[0], buffer, sizeof(buffer))) > 0) {
        response.append(buffer, bytesRead);
    }
    
    close(outputPipe[0]);

    // Attend la fin du processus CGI
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        throw std::runtime_error("Erreur execution CGI");
    }

    return response;
}