#ifndef LgBackendController_DEFINED
#define LgBackendController_DEFINED

#include <string>
#include <vector>

namespace SDK {

class ResourceRequest {
private:
    std::string url;

public:
    ResourceRequest(const std::string& url);
    const std::string& getUrl() const { return url; }
    virtual ~ResourceRequest();
};

class ResourceResponse {
private:
    std::vector<uint8_t> data;
    std::string mimeType;
public:
    ResourceResponse(const std::vector<uint8_t>& data,
                     const std::string& mimeType);
    ResourceResponse();
    ResourceResponse(const ResourceResponse& other);

    const std::vector<uint8_t>& getData() const { return data; } 
    const std::string& getMimeType() const { return mimeType; }

    void setData(const std::vector<uint8_t>& data) { this->data = data; }
    void setMimeType(const std::string& mimeType) { this->mimeType = mimeType; }

    virtual ~ResourceResponse();
};

class Backend {
public:
    Backend();
    virtual ~Backend();

    virtual ResourceResponse ProcessRequest(const ResourceRequest& request);
};

}

#endif