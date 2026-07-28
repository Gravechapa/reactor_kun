module;
#include <base64.hpp>
#include <nlohmann/json.hpp>

module JoyReactorApi;

static const std::string_view query{
    R"(query ($id: ID!) {
  node(id: $id)
  {
    ... on Post{
      id
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
      }
    }
  }
}
)"};

std::string JoyReactorApi::postQuery(std::string_view id)
{
    return nlohmann::json{{"query", query},
                          {"variables", nlohmann::json{{"id", base64::to_base64(std::format("Post:{}", id))}}}}
        .dump();
}
