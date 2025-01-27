/***************************************************************************
 *
 * Project         _____    __   ____   _      _
 *                (  _  )  /__\ (_  _)_| |_  _| |_
 *                 )(_)(  /(__)\  )( (_   _)(_   _)
 *                (_____)(__)(__)(__)  |_|    |_|
 *
 *
 * Copyright 2018-present, Leonid Stryzhevskyi <lganzzzo@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************************/

#include "TreeToObjectMapperTest.hpp"

#include "oatpp/json/ObjectMapper.hpp"

#include "oatpp/data/mapping/TreeToObjectMapper.hpp"

#include "oatpp/macro/codegen.hpp"

#include <iostream>

namespace oatpp { namespace data { namespace mapping {

namespace {

#include OATPP_CODEGEN_BEGIN(DTO)

class TestDto1 : public oatpp::DTO {

  DTO_INIT(TestDto1, DTO)

  DTO_FIELD(String, str);

  DTO_FIELD(Int8, i8);
  DTO_FIELD(UInt8, ui8);

  DTO_FIELD(Int16, i16);
  DTO_FIELD(UInt16, ui16);

  DTO_FIELD(Int32, i32);
  DTO_FIELD(UInt32, ui32);

  DTO_FIELD(Int64, i64);
  DTO_FIELD(UInt64, ui64);

  DTO_FIELD(Vector<oatpp::Object<TestDto1>>, vector);
  DTO_FIELD(UnorderedFields<oatpp::Object<TestDto1>>, map);
  DTO_FIELD(Fields<String>, pairs);

};

#include OATPP_CODEGEN_END(DTO)

}

void TreeToObjectMapperTest::onRun() {

  json::ObjectMapper jsonMapper;
  jsonMapper.serializerConfig().json.useBeautifier = true;
  jsonMapper.serializerConfig().mapper.includeNullFields = false;

  TreeToObjectMapper mapper;
  TreeToObjectMapper::Config config;

  ObjectToTreeMapper reverseMapper;
  ObjectToTreeMapper::Config reverseConfig;

  {
    Tree tree;

    tree["str"] = "Hello World!";
    tree["i8"] = -8;
    tree["ui8"] = 8;
    tree["i16"] = -16;
    tree["ui16"] = 16;
    tree["i32"] = -32;
    tree["ui32"] = 32;
    tree["i64"] = -64;
    tree["ui64"] = 64;

    tree["vector"].setVector(3);
    tree["vector"][0]["str"] = "nested_1 (in vector)";
    tree["vector"][1]["str"] = "nested_2 (in vector)";
    tree["vector"][2]["str"] = "nested_3 (in vector)";

    tree["map"]["nested_1"]["i32"] = 1;
    tree["map"]["nested_2"]["i32"] = 2;
    tree["map"]["nested_3"]["i32"] = 3;

    auto& pairs = tree["pairs"].getPairs();
    pairs.push_back({"same-key", {}});
    pairs.push_back({"same-key", {}});
    pairs.push_back({"same-key", {}});

    pairs[0].second = "value1";
    pairs[1].second = "value2";
    pairs[2].second = "value3";

    TreeToObjectMapper::State state;
    state.config = &config;
    state.tree = &tree;

    const auto& polymorph = mapper.map(state,oatpp::Object<TestDto1>::Class::getType());

    if(state.errorStack.empty()) {
      auto json = jsonMapper.writeToString(polymorph);
      std::cout << *json << std::endl;
    } else {
      auto err = state.errorStack.stacktrace();
      std::cout << *err << std::endl;
    }

    auto obj = polymorph.cast<oatpp::Object<TestDto1>>();

    OATPP_ASSERT(obj->str == "Hello World!")
    OATPP_ASSERT(obj->i8 == -8)
    OATPP_ASSERT(obj->ui8 == 8)
    OATPP_ASSERT(obj->i16 == -16)
    OATPP_ASSERT(obj->ui16 == 16)
    OATPP_ASSERT(obj->i32 == -32)
    OATPP_ASSERT(obj->ui32 == 32)
    OATPP_ASSERT(obj->i64 == -64)
    OATPP_ASSERT(obj->ui64 == 64)

    OATPP_ASSERT(obj->vector->size() == 3)
    OATPP_ASSERT(obj->vector[0]->str == "nested_1 (in vector)")
    OATPP_ASSERT(obj->vector[1]->str == "nested_2 (in vector)")
    OATPP_ASSERT(obj->vector[2]->str == "nested_3 (in vector)")

    OATPP_ASSERT(obj->map->size() == 3)
    OATPP_ASSERT(obj->map["nested_1"]->i32 = 1)
    OATPP_ASSERT(obj->map["nested_2"]->i32 = 2)
    OATPP_ASSERT(obj->map["nested_3"]->i32 = 3)

    OATPP_ASSERT(obj->pairs->size() == 3)
    OATPP_ASSERT(obj->pairs[0].first == "same-key" && obj->pairs[0].second == "value1")
    OATPP_ASSERT(obj->pairs[1].first == "same-key" && obj->pairs[1].second == "value2")
    OATPP_ASSERT(obj->pairs[2].first == "same-key" && obj->pairs[2].second == "value3")

  }

}

}}}
