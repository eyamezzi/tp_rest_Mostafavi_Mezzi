#include "ServicesManager.h"

#include <charconv>
#include <optional>
#include <utility>

void ServicesManager::registerService(std::unique_ptr<AbstractService> service)
{
    services_.push_back(std::move(service));
}
AbstractService* ServicesManager::findService(std::string_view url) const
{
    for (const auto& service : services_) {
        const std::string& pattern = service->getPattern();
        if (url.starts_with(pattern)
            && (url.size() == pattern.size() || url[pattern.size()] == '/'))
            return service.get();
    }
    return nullptr;
}
http::status ServicesManager::queryService(std::string& out, const std::string& in,
                                           std::string_view url, http::verb method)
{
    if (auto pos = url.find('?'); pos != std::string_view::npos)
        url = url.substr(0, pos);

    AbstractService* service = findService(url);
    if (service == nullptr)
        throw ServiceException(http::status::not_found, "Invalid service");

    std::optional<int> id;
    std::string_view rest = url.substr(service->getPattern().size());
    if (!rest.empty() && rest != "/") {
        rest.remove_prefix(1);
        int value = 0;
        const char* end = rest.data() + rest.size();
        auto [ptr, ec] = std::from_chars(rest.data(), end, value);
        if (ec != std::errc{} || ptr != end)
            throw ServiceException(http::status::bad_request, "Invalid id");
        id = value;
    }

    auto parseBody = [&in]() {
        boost::system::error_code ec;
        json::value jin = json::parse(in, ec);
        if (ec)
            throw ServiceException(http::status::bad_request, "Invalid JSON body");
        return jin;
    };

    // Aiguillage selon la méthode
    json::value jout;
    http::status status;

    switch (method) {
        case http::verb::get:
            status = service->get(jout, id);
            break;

        case http::verb::post:
            if (id)
                throw ServiceException(http::status::method_not_allowed, "POST must target the collection");
            status = service->post(jout, parseBody());
            break;

        case http::verb::patch:
            status = service->patch(id, parseBody());
            break;

        case http::verb::delete_:
            status = service->remove(id);
            break;
        case http::verb::put:                                      
            status = service->put(id, parseBody());
            break;
        default:
            throw ServiceException(http::status::not_implemented, "Method not implemented");
    }

    if (!jout.is_null())
        out = json::serialize(jout);
    return status;
}