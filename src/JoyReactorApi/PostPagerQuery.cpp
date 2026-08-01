module;
#include <nlohmann/json.hpp>

module JoyReactorApi;

static constexpr std::string_view query{
    R"(query ($name: String
       $lineType: PostLineType!
       $page: Int) {{
  tag(name: $name){{
    postPager(type: $lineType){{
      count
      posts(page: $page){{
        {}
      }}
    }}
  }}
}}
)"};

std::string JoyReactorApi::postPagerQuery(std::string_view tagName, std::string_view popularity, uint32_t page)
{
    nlohmann::json vars;
    if (tagName.empty())
    {
        vars["name"] = nullptr;
    }
    else
    {
        vars["name"] = tagName;
    }
    vars["lineType"] = popularity;
    if (page)
    {
        vars["page"] = page;
    }
    else
    {
        vars["page"] = nullptr;
    }
    return nlohmann::json{{"query", std::format(query, _queryPostBody)}, {"variables", vars}}.dump();
}
