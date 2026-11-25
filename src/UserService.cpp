#include "UserService.h"

UserService::UserService(UserDB& userDB) : AbstractService("/user"), userDB_(userDB) {}


http::status UserService::get(json::value& out, std::optional<int> id) const
{
    // GET /user :liste de tous les utilisateurs
    if (!id) {
        json::array list;
        for (const auto& [userId, user] : userDB_.getUsers()) {
            json::value v = json::value_from(user);
            v.as_object()["id"] = userId;
            list.push_back(std::move(v));
        }
        out = std::move(list);
        return http::status::ok;
    }

    // GET /user/<id> -> un seul utilisateur
    const User* user = userDB_.getUser(*id);
    if (user == nullptr)
        throw ServiceException(http::status::not_found, "Invalid user id");

    out = json::value_from(*user);
    return http::status::ok;
}


http::status UserService::post(json::value& out, const json::value& in)
{

    if (!in.is_object())
        throw ServiceException(http::status::bad_request, "Body must be a JSON object");
    const json::object& obj = in.as_object();


    const json::value* name = obj.if_contains("name");
    if (name == nullptr || !name->is_string() || name->as_string().empty())
        throw ServiceException(http::status::bad_request, "Invalid name");


    const json::value* age = obj.if_contains("age");
    if (age == nullptr || !age->is_int64())
        throw ServiceException(http::status::bad_request, "Invalid age");
    std::int64_t ageValue = age->as_int64();
    if (ageValue < 0 || ageValue > 150)
        throw ServiceException(http::status::bad_request, "Invalid age");

    // Créer l'utilisateur et l'ajouter à la base
    User user{std::string(name->as_string()), static_cast<int>(ageValue)};
    int id = userDB_.addUser(user);
    out = {{"id", id}};
    return http::status::created;
}

http::status UserService::patch(std::optional<int> id, const json::value& in)
{
    if (!id)
        throw ServiceException(http::status::method_not_allowed, "PATCH needs a user id");

    const User* current = userDB_.getUser(*id);
    if (current == nullptr)
        throw ServiceException(http::status::not_found, "Invalid user id");

    if (!in.is_object())
        throw ServiceException(http::status::bad_request, "Body must be a JSON object");

    User updated = *current;

    for (const auto& [cle, valeur] : in.as_object()) {
        if (cle == "name") {
            if (!valeur.is_string() || valeur.as_string().empty())
                throw ServiceException(http::status::bad_request, "Invalid name");
            updated.name = std::string(valeur.as_string());
        }
        else if (cle == "age") {
            if (!valeur.is_int64())
                throw ServiceException(http::status::bad_request, "Invalid age");
            std::int64_t ageValue = valeur.as_int64();
            if (ageValue < 0 || ageValue > 150)
                throw ServiceException(http::status::bad_request, "Invalid age");
            updated.age = static_cast<int>(ageValue);
        }
        else {
            throw ServiceException(http::status::bad_request, "Unknown field");
        }
    }

    userDB_.setUser(*id, updated);
    return http::status::no_content;
}

http::status UserService::remove(std::optional<int> id)
{
    if (!id)
        throw ServiceException(http::status::method_not_allowed, "DELETE needs a user id");

    if (!userDB_.removeUser(*id))
        throw ServiceException(http::status::not_found, "Invalid user id");

    return http::status::no_content;
}
http::status UserService::put(std::optional<int> id, const json::value& in)
{
    if (!id)
        throw ServiceException(http::status::method_not_allowed, "PUT needs a user id");

    if (userDB_.getUser(*id) == nullptr)
        throw ServiceException(http::status::not_found, "Invalid user id");

    if (!in.is_object())
        throw ServiceException(http::status::bad_request, "Body must be a JSON object");
    const json::object& obj = in.as_object();

   
    if (obj.size() != 2)
        throw ServiceException(http::status::bad_request, "PUT requires exactly name and age");

    const json::value* name = obj.if_contains("name");
    if (name == nullptr || !name->is_string() || name->as_string().empty())
        throw ServiceException(http::status::bad_request, "Invalid name");

    const json::value* age = obj.if_contains("age");
    if (age == nullptr || !age->is_int64())
        throw ServiceException(http::status::bad_request, "Invalid age");
    std::int64_t ageValue = age->as_int64();
    if (ageValue < 0 || ageValue > 150)
        throw ServiceException(http::status::bad_request, "Invalid age");

    User user{std::string(name->as_string()), static_cast<int>(ageValue)};
    userDB_.setUser(*id, user);
    return http::status::no_content;
}
