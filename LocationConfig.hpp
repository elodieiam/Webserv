#pragma once
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

class LocationConfig
{
public:
    LocationConfig();
    LocationConfig(LocationConfig const &other);
    LocationConfig &operator=(LocationConfig const &other);
    ~LocationConfig();

    void setPath(const std::string &p);
    void addAllowMethod(const std::string &method);
    void setIndex(const std::string &idx);
    void setRoot(const std::string &rt);
    void addCgiExtension(const std::string &ext);
    void addCgiPath(const std::string &ext, const std::string &path);
    void setUploadDir(const std::string &dir);

    std::string getPath() const;
    std::vector<std::string> getAllowMethods() const;
    std::string getIndex() const;
    std::string getRoot() const;
    std::vector<std::string> getCgiExtensions() const;
    std::map<std::string, std::string> getCgiPaths() const;
    std::string getUploadDir() const;

private:

    std::string path;
    std::vector<std::string> allow_methods;
    std::string index;
    std::string root;
    std::vector<std::string> cgi_extension;
    std::map<std::string, std::string> cgi_path;
    std::string upload_dir;
};