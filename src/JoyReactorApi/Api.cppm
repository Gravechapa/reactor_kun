export module JoyReactorApi;
import std;

export class JoyReactorApi
{
  public:
    static std::string postQuery(std::string_view id);
    static std::string postPagerQuery(std::string_view tagName, std::string_view popularity, uint32_t page);

  private:
    static const std::string_view _queryPostBody;
};
