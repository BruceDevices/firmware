// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2026, Benoit BLANCHON
// MIT License

#pragma once

#include <ArduinoJson/Variant/VariantRefBase.hpp>

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

// A proxy class to get or set a member of an object.
// https://arduinojson.org/v7/api/jsonobject/subscript/
template <typename TUpstream, typename AdaptedString>
class MemberProxy
    : public VariantRefBase<MemberProxy<TUpstream, AdaptedString>>,
      public VariantOperators<MemberProxy<TUpstream, AdaptedString>> {
  friend class VariantAttorney;

  friend class VariantRefBase<MemberProxy<TUpstream, AdaptedString>>;

  template <typename, typename>
  friend class MemberProxy;

  template <typename>
  friend class ElementProxy;

 public:
  MemberProxy(TUpstream upstream, AdaptedString key)
      : upstream_(upstream), key_(key) {}

  MemberProxy& operator=(const MemberProxy& src) {
    this->set(src);
    return *this;
  }

  template <typename T>
  MemberProxy& operator=(const T& src) {
    this->set(src);
    return *this;
  }

  template <typename T, enable_if_t<!is_const<T>::value, int> = 0>
  MemberProxy& operator=(T* src) {
    this->set(src);
    return *this;
  }

 private:
  // clang-format off
  MemberProxy(const MemberProxy& src) // Error here? See https://arduinojson.org/v7/proxy-non-copyable/
      : upstream_(src.upstream_), key_(src.key_) {}
  // clang-format on

  ResourceManager* getResourceManager() const {
    return VariantAttorney::getResourceManager(upstream_);
  }

  VariantData* getData() const {
    return VariantData::getMember(
        VariantAttorney::getData(upstream_), key_,
        VariantAttorney::getResourceManager(upstream_));
  }

  VariantData* getOrCreateData() const {
    auto data = VariantAttorney::getOrCreateData(upstream_);
    if (!data)
      return nullptr;
    return data->getOrAddMember(key_,
                                VariantAttorney::getResourceManager(upstream_));
  }

 private:
  TUpstream upstream_;
  AdaptedString key_;
};

ARDUINOJSON_END_PRIVATE_NAMESPACE
