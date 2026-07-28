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
      createdAt
      nsfw
      rating
      text
      unsafe
      user{
        username
      }
      attributes{
        id
        type
        insertId
        ... on PostAttributeEmbed{
          value
        }
        ... on PostAttributePicture{
          image{
            width
            height
            comment
            type
            hasVideo
          }
        }
      }
      tags{
        name
      }
      bestComments{
        text
        createdAt
        rating
        level
        user{
            username
        }
        attributes{
          id
          type
          insertId
          ... on CommentAttributeEmbed{
            value
          }
          ... on CommentAttributePicture{
            image{
              width
              height
              comment
              type
              hasVideo
            }
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
