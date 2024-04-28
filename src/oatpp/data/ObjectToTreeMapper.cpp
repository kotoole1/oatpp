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

#include "ObjectToTreeMapper.hpp"

#include "oatpp/data/stream/BufferStream.hpp"
#include "oatpp/utils/Conversion.hpp"

namespace oatpp { namespace data {

oatpp::String ObjectToTreeMapper::MappingState::errorStacktrace() const {
  stream::BufferOutputStream ss;
  for(auto& s : errorStack) {
    ss << s << "\n";
  }
  return ss.toString();
}

ObjectToTreeMapper::ObjectToTreeMapper() {

  m_methods.resize(static_cast<size_t>(data::mapping::type::ClassId::getClassCount()), nullptr);

  setMapperMethod(data::mapping::type::__class::String::CLASS_ID, &ObjectToTreeMapper::mapString);
  setMapperMethod(data::mapping::type::__class::Any::CLASS_ID, &ObjectToTreeMapper::mapAny);

  setMapperMethod(data::mapping::type::__class::Int8::CLASS_ID, &ObjectToTreeMapper::mapPrimitive<oatpp::Int8>);
  setMapperMethod(data::mapping::type::__class::UInt8::CLASS_ID, &ObjectToTreeMapper::mapPrimitive<oatpp::UInt8>);

  setMapperMethod(data::mapping::type::__class::Int16::CLASS_ID, &ObjectToTreeMapper::mapPrimitive<oatpp::Int16>);
  setMapperMethod(data::mapping::type::__class::UInt16::CLASS_ID, &ObjectToTreeMapper::mapPrimitive<oatpp::UInt16>);

  setMapperMethod(data::mapping::type::__class::Int32::CLASS_ID, &ObjectToTreeMapper::mapPrimitive<oatpp::Int32>);
  setMapperMethod(data::mapping::type::__class::UInt32::CLASS_ID, &ObjectToTreeMapper::mapPrimitive<oatpp::UInt32>);

  setMapperMethod(data::mapping::type::__class::Int64::CLASS_ID, &ObjectToTreeMapper::mapPrimitive<oatpp::Int64>);
  setMapperMethod(data::mapping::type::__class::UInt64::CLASS_ID, &ObjectToTreeMapper::mapPrimitive<oatpp::UInt64>);

  setMapperMethod(data::mapping::type::__class::Float32::CLASS_ID, &ObjectToTreeMapper::mapPrimitive<oatpp::Float32>);
  setMapperMethod(data::mapping::type::__class::Float64::CLASS_ID, &ObjectToTreeMapper::mapPrimitive<oatpp::Float64>);
  setMapperMethod(data::mapping::type::__class::Boolean::CLASS_ID, &ObjectToTreeMapper::mapPrimitive<oatpp::Boolean>);

  setMapperMethod(data::mapping::type::__class::AbstractObject::CLASS_ID, &ObjectToTreeMapper::mapObject);
  setMapperMethod(data::mapping::type::__class::AbstractEnum::CLASS_ID, &ObjectToTreeMapper::mapEnum);

  setMapperMethod(data::mapping::type::__class::AbstractVector::CLASS_ID, &ObjectToTreeMapper::mapCollection);
  setMapperMethod(data::mapping::type::__class::AbstractList::CLASS_ID, &ObjectToTreeMapper::mapCollection);
  setMapperMethod(data::mapping::type::__class::AbstractUnorderedSet::CLASS_ID, &ObjectToTreeMapper::mapCollection);

  setMapperMethod(data::mapping::type::__class::AbstractPairList::CLASS_ID, &ObjectToTreeMapper::mapMap);
  setMapperMethod(data::mapping::type::__class::AbstractUnorderedMap::CLASS_ID, &ObjectToTreeMapper::mapMap);

}

void ObjectToTreeMapper::setMapperMethod(const data::mapping::type::ClassId& classId, MapperMethod method) {
  const auto id = static_cast<v_uint32>(classId.id);
  if(id >= m_methods.size()) {
    m_methods.resize(id + 1, nullptr);
  }
  m_methods[id] = method;
}

void ObjectToTreeMapper::map(MappingState& state, const oatpp::Void& polymorph)
{
  auto id = static_cast<v_uint32>(polymorph.getValueType()->classId.id);
  auto& method = m_methods[id];
  if(method) {
    (*method)(this, state, polymorph);
  } else {
    auto* interpretation = polymorph.getValueType()->findInterpretation(state.config->enabledInterpretations);
    if(interpretation) {
      map(state, interpretation->toInterpretation(polymorph));
    } else {

      state.errorStack.emplace_back("[oatpp::data::ObjectToTreeMapper::map()]: "
                                    "Error. No serialize method for type '" +
                                    oatpp::String(polymorph.getValueType()->classId.name) + "'");

      return;
    }
  }
}

void ObjectToTreeMapper::mapString(ObjectToTreeMapper* mapper, MappingState& state, const oatpp::Void& polymorph) {
  if(!polymorph) {
    state.tree->setNull();
    return;
  }
  state.tree->setString(oatpp::String(std::static_pointer_cast<std::string>(polymorph.getPtr()), oatpp::String::Class::getType()));
}

void ObjectToTreeMapper::mapAny(ObjectToTreeMapper* mapper, MappingState& state, const oatpp::Void& polymorph) {
  if(!polymorph) {
    state.tree->setNull();
    return;
  }
  auto anyHandle = static_cast<data::mapping::type::AnyHandle*>(polymorph.get());
  mapper->map(state, oatpp::Void(anyHandle->ptr, anyHandle->type));
}

void ObjectToTreeMapper::mapEnum(ObjectToTreeMapper* mapper, MappingState& state, const oatpp::Void& polymorph) {

  if(!polymorph) {
    state.tree->setNull();
    return;
  }

  auto polymorphicDispatcher = static_cast<const data::mapping::type::__class::AbstractEnum::PolymorphicDispatcher*>(
    polymorph.getValueType()->polymorphicDispatcher
  );

  data::mapping::type::EnumInterpreterError e = data::mapping::type::EnumInterpreterError::OK;
  mapper->map(state, polymorphicDispatcher->toInterpretation(polymorph, e));

  if(e == data::mapping::type::EnumInterpreterError::OK) {
    return;
  }

  switch(e) {
    case data::mapping::type::EnumInterpreterError::CONSTRAINT_NOT_NULL:
      state.errorStack.emplace_back("[oatpp::data::ObjectToTreeMapper::mapEnum()]: Error. Enum constraint violated - 'NotNull'.");
      break;
    case data::mapping::type::EnumInterpreterError::OK:
    case data::mapping::type::EnumInterpreterError::TYPE_MISMATCH_ENUM:
    case data::mapping::type::EnumInterpreterError::TYPE_MISMATCH_ENUM_VALUE:
    case data::mapping::type::EnumInterpreterError::ENTRY_NOT_FOUND:
    default:
      state.errorStack.emplace_back("[oatpp::data::ObjectToTreeMapper::mapEnum()]: Error. Can't serialize Enum.");
  }

}

void ObjectToTreeMapper::mapCollection(ObjectToTreeMapper* mapper, MappingState& state, const oatpp::Void& polymorph) {

  if(!polymorph) {
    state.tree->setNull();
    return;
  }

  auto dispatcher = static_cast<const data::mapping::type::__class::Collection::PolymorphicDispatcher*>(
    polymorph.getValueType()->polymorphicDispatcher
  );

  auto iterator = dispatcher->beginIteration(polymorph);

  state.tree->setVector(0);
  auto& vector = state.tree->getVector();
  v_int64 index = 0;

  while (!iterator->finished()) {

    const auto& value = iterator->get();

    if(value || state.config->includeNullFields || state.config->alwaysIncludeNullCollectionElements) {

      vector.emplace_back();

      MappingState nestedState;
      nestedState.tree = &vector[vector.size() - 1];
      nestedState.config = state.config;

      mapper->map(nestedState, value);

      if(!nestedState.errorStack.empty()) {
        state.errorStack.splice(state.errorStack.end(), nestedState.errorStack);
        state.errorStack.emplace_back("[oatpp::data::ObjectToTreeMapper::mapCollection()]: index=" + utils::Conversion::int64ToStr(index));
        return;
      }

    }

    iterator->next();
    index ++;

  }

}

void ObjectToTreeMapper::mapMap(ObjectToTreeMapper* mapper, MappingState& state, const oatpp::Void& polymorph) {

  if(!polymorph) {
    state.tree->setNull();
    return;
  }

  auto dispatcher = static_cast<const data::mapping::type::__class::Map::PolymorphicDispatcher*>(
    polymorph.getValueType()->polymorphicDispatcher
  );

  auto keyType = dispatcher->getKeyType();
  if(keyType->classId != oatpp::String::Class::CLASS_ID){
    state.errorStack.emplace_back("[oatpp::data::ObjectToTreeMapper::mapMap()]: Invalid map key. Key should be String");
    return;
  }

  auto iterator = dispatcher->beginIteration(polymorph);

  state.tree->setMap({});
  auto& map = state.tree->getMap();

  while (!iterator->finished()) {
    const auto& value = iterator->getValue();
    if(value || state.config->includeNullFields || state.config->alwaysIncludeNullCollectionElements) {

      const auto& untypedKey = iterator->getKey();
      const auto& key = oatpp::String(std::static_pointer_cast<std::string>(untypedKey.getPtr()));

      MappingState nestedState;
      nestedState.tree = &map[key];
      nestedState.config = state.config;

      mapper->map(nestedState, value);

      if(!nestedState.errorStack.empty()) {
        state.errorStack.splice(state.errorStack.end(), nestedState.errorStack);
        state.errorStack.emplace_back("[oatpp::data::ObjectToTreeMapper::mapMap()]: key='" + key + "'");
        return;
      }

    }
    iterator->next();
  }

}

void ObjectToTreeMapper::mapObject(ObjectToTreeMapper* mapper, MappingState& state, const oatpp::Void& polymorph) {

  if(!polymorph) {
    state.tree->setNull();
    return;
  }

  auto type = polymorph.getValueType();
  auto dispatcher = static_cast<const oatpp::data::mapping::type::__class::AbstractObject::PolymorphicDispatcher*>(
    type->polymorphicDispatcher
  );
  auto fields = dispatcher->getProperties()->getList();
  auto object = static_cast<oatpp::BaseObject*>(polymorph.get());

  state.tree->setMap({});
  auto& map = state.tree->getMap();

  for (auto const& field : fields) {

    oatpp::Void value;
    if(field->info.typeSelector && field->type == oatpp::Any::Class::getType()) {
      const auto& any = field->get(object).cast<oatpp::Any>();
      value = any.retrieve(field->info.typeSelector->selectType(object));
    } else {
      value = field->get(object);
    }

    if(field->info.required && value == nullptr) {
      state.errorStack.emplace_back("[oatpp::data::ObjectToTreeMapper::mapObject()]: "
                                    "Error. " + std::string(type->nameQualifier) + "::"
                                    + std::string(field->name) + " is required!");
      return;
    }

    if (value || state.config->includeNullFields || (field->info.required && state.config->alwaysIncludeRequired)) {

      MappingState nestedState;
      nestedState.tree = &map[oatpp::String(field->name)];
      nestedState.config = state.config;

      mapper->map(nestedState, value);

      if(!nestedState.errorStack.empty()) {
        state.errorStack.splice(state.errorStack.end(), nestedState.errorStack);
        state.errorStack.emplace_back("[oatpp::data::ObjectToTreeMapper::mapObject()]: field='" + oatpp::String(field->name) + "'");
        return;
      }

    }

  }


}

}}
