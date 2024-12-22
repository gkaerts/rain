#pragma once

#include <cstddef>
#include <type_traits>
#include <cstdint>

#include "common/type_id.hpp"

namespace rn::meta
{
    template<class... Args>
    struct ArgList
    {};


    template<class ArgList>
    struct Size{};

    template<template<class...> typename ArgList, class... Args>
    struct Size<ArgList<Args...>>
    {
        static constexpr std::size_t value = sizeof...(Args);
    };

    template<typename ArgList, class Element>
    struct PushFront{};


    template<template<class...> typename ArgList, class... Args, class Element>
    struct PushFront<ArgList<Args...>, Element>
    {
        using type = ArgList<Element, Args...>;
    };


    template<typename Left, typename Right, typename = void>
    struct Merge
    {};


    template<template<class...> typename Left, template<class...> typename Right, class F1, class F2, class... Args1, class... Args2>
    struct Merge<Left<F1, Args1...>, Right<F2, Args2...>, std::enable_if_t<!(TypeID<F1>() > TypeID<F2>())>>
    {
        using type = typename PushFront<typename Merge<Left<Args1...>, Right<F2, Args2...>>::type, F1>::type;
    };


    template<template<class...> typename Left, template<class...> typename Right, class F1, class F2, class... Args1, class... Args2>
    struct Merge<Left<F1, Args1...>, Right<F2, Args2...>, std::enable_if_t<(TypeID<F1>() > TypeID<F2>())>>
    {
        using type = typename PushFront<typename Merge<Left<F1, Args1...>, Right<Args2...>>::type, F2>::type;
    };


    template<template<class...> typename Left, template<class...> typename Right, class... Args>
    struct Merge<Left<>, Right<Args...>>
    {
        using type = Right<Args...>;
    };


    template<template<class...> typename Left, template<class...> typename Right, class... Args>
    struct Merge<Left<Args...>, Right<>>
    {
        using type = Left<Args...>;
    };


    template<template<class...> typename Left, template<class...> typename Right>
    struct Merge<Left<>, Right<>>
    {
        using type = Left<>;
    };


    template<typename ArgList, std::size_t Num, typename = void>
    struct Car{};


    template<template<class...> typename ArgList, class... Args>
    struct Car<ArgList<Args...>, 0>
    {
        using type = ArgList<>;
    };


    template<template<class...> typename ArgList, class First, class... Args, std::size_t Num>
    struct Car<ArgList<First, Args...>, Num, std::enable_if_t<Num != 0>>
    {
        using type = typename PushFront<typename Car<ArgList<Args...>, Num - 1>::type, First>::type;
    };


    template<typename ArgList, std::size_t Num, typename = void>
    struct Cdr{};


    template<template<class...> typename ArgList, class... Args>
    struct Cdr<ArgList<Args...>, 0>
    {
        using type = ArgList<Args...>;
    };


    template<template<class...> typename ArgList, class First, class... Args, std::size_t Num>
    struct Cdr<ArgList<First, Args...>, Num, std::enable_if_t<Num != 0>>
    {
        using type = typename Cdr<ArgList<Args...>, Num - 1>::type;
    };


    template<typename ArgList>
    struct PackSort
    {
        using type = typename Merge<typename PackSort<typename Car<ArgList, Size<ArgList>::value / 2>::type>::type
                                    , typename PackSort<typename Cdr<ArgList, Size<ArgList>::value / 2>::type>::type>::type;
    };


    template<template<class...> typename ArgList, class Arg>
    struct PackSort<ArgList<Arg>>
    {
        using type = ArgList<Arg>;
    };


    template<template<class...> typename ArgList>
    struct PackSort<ArgList<>>
    {
        using type = ArgList<>;
    };

    template<typename ArgList>
    using PackSortT = PackSort<ArgList>::type;

    template <template <class...> class Ins, class...> struct Instantiate;

    template <template <class...> class Ins, class... Args> 
    struct Instantiate<Ins, ArgList<Args...>> {
        using type = Ins<Args...>;
    };

    template <template <class...> class Ins, class... Args> 
    using InstantiateT = Instantiate<Ins, Args...>::type;

    template <template <class...> class Ins, class... Args>
    using InstantiateSortedT = InstantiateT<Ins, PackSortT<ArgList<Args...>>>;
}