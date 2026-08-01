module;
#include <base64.hpp>
#include <nlohmann/json.hpp>

module JoyReactorApi;

const std::string_view JoyReactorApi::_queryPostBody{
    R"(id
      tags{
        name
      }
      createdAt
      user{
        username
      }
      rating
      nsfw
      unsafe
      text
      attributes{
        insertId
        type
        ... on PostAttributeEmbed{
          value
        }
        ... on PostAttributePicture{
          id
          image{
            type
            width
            height
            hasVideo
          }
        }
      }
      poll{
        question
        answers{
          answer
          count
        }
      }
      bestComments{
        id
        createdAt
        user{
            username
        }
        level
        rating
        text
        attributes{
          insertId
          type
          ... on CommentAttributeEmbed{
            value
          }
          ... on CommentAttributePicture{
            id
            image{
              type
              width
              height
              hasVideo
            }
          }
        }
      })"};
static constexpr std::string_view query{
    R"(query ($id: ID!) {{
  node(id: $id)
  {{
    ... on Post{{
      {}
    }}
  }}
}}
)"};

std::string JoyReactorApi::postQuery(std::string_view id)
{
    return nlohmann::json{{"query", std::format(query, _queryPostBody)},
                          {"variables", nlohmann::json{{"id", base64::to_base64(std::format("Post:{}", id))}}}}
        .dump();
}
