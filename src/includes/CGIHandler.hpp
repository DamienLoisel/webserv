#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

class CGIHandler {
private:
    std::string _scriptPath;
    std::string _requestBody;
    std::map<std::string, std::string> _env;

    void setupEnvironment(const std::string& method, 
                         const std::string& queryString,
                         const std::string& contentType = "");
    char** createEnvArray();
    void cleanupEnvArray(char** env);

public:
    CGIHandler(const std::string& scriptPath);
    std::string executeCGI(const std::string& method, 
                          const std::string& queryString = "", 
                          const std::string& requestBody = "",
                          const std::string& contentType = "text/plain");
};

#endif