// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2026, Benoit BLANCHON
// MIT License

#pragma once

#include <ArduinoJson/Polyfills/utility.hpp>

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

// a meta function that tells if the type is a string literal (const char[N])
template <typename T>
struct IsStringLiteral : false_type {};

template <size_t N>
struct IsStringLiteral<const char (&)[N]> : true_type {};

template <typename TString, typename Enable = void>
struct StringAdapter;

template <typename TString, typename Enable = void>
struct SizedStringAdapter;

template <typename TString>
using StringAdapterFor =
    StringAdapter<conditional_t<IsStringLiteral<TString>::value, TString,
                                remove_cv_t<remove_reference_t<TString>>>>;

template <typename T>
using AdaptedString = typename StringAdapterFor<T>::AdaptedString;

template <typename TString>
AdaptedString<TString> adaptString(TString&& s) {
  return StringAdapterFor<TString>::adapt(detail::forward<TString>(s));
}

template <typename TChar, enable_if_t<!is_const<TChar>::value, int> = 0>
AdaptedString<TChar*> adaptString(TChar* p) {
  return StringAdapter<TChar*>::adapt(p);
}

template <typename TChar>
AdaptedString<TChar*> adaptString(TChar* p, size_t n) {
  return SizedStringAdapter<TChar*>::adapt(p, n);
}

ARDUINOJSON_END_PRIVATE_NAMESPACE
