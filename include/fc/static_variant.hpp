/** This source adapted from https://github.com/kmicklas/variadic-static_variant
 *
 * Copyright (C) 2013 Kenneth Micklas
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **/
#pragma once

#include <functional>
#include <stdexcept>
#include <typeinfo>

#include <fc/array.hpp>
#include <fc/exception/exception.hpp>

namespace fc {

// Implementation details, the user should not import this:
namespace impl {

template<int N, typename... Ts>
struct storage_ops;

template<typename X, typename... Ts>
struct position;

template<typename... Ts>
struct type_info;

template<typename StaticVariant>
struct copy_construct
{
   typedef void result_type;
   StaticVariant& sv;
   copy_construct( StaticVariant& s ):sv(s){}
   template<typename T>
   void operator()( const T& v )const
   {
      sv.init(v);
   }
};

template<typename StaticVariant>
struct move_construct
{
   typedef void result_type;
   StaticVariant& sv;
   move_construct( StaticVariant& s ):sv(s){}
   template<typename T>
   void operator()( T& v )const
   {
      sv.init( std::move(v) );
   }
};

template<int N, typename T, typename... Ts>
struct storage_ops<N, T&, Ts...> {
    static void del(int n, void *data) {}
    static void con(int n, void *data) {}

    template<typename visitor>
    static typename visitor::result_type apply(int n, void *data, visitor& v) {}

    template<typename visitor>
    static typename visitor::result_type apply(int n, void *data, const visitor& v) {}

    template<typename visitor>
    static typename visitor::result_type apply(int n, const void *data, visitor& v) {}

    template<typename visitor>
    static typename visitor::result_type apply(int n, const void *data, const visitor& v) {}
};

template<int N, typename T, typename... Ts>
struct storage_ops<N, T, Ts...> {
    static void del(int n, void *data) {
        if(n == N) reinterpret_cast<T*>(data)->~T();
        else storage_ops<N + 1, Ts...>::del(n, data);
    }
    static void con(int n, void *data) {
        if(n == N) new(reinterpret_cast<T*>(data)) T();
        else storage_ops<N + 1, Ts...>::con(n, data);
    }

    template<typename visitor>
    static typename visitor::result_type apply(int n, void *data, visitor& v) {
        if(n == N) return v(*reinterpret_cast<T*>(data));
        else return storage_ops<N + 1, Ts...>::apply(n, data, v);
    }

    template<typename visitor>
    static typename visitor::result_type apply(int n, void *data, const visitor& v) {
        if(n == N) return v(*reinterpret_cast<T*>(data));
        else return storage_ops<N + 1, Ts...>::apply(n, data, v);
    }

    template<typename visitor>
    static typename visitor::result_type apply(int n, const void *data, visitor& v) {
        if(n == N) return v(*reinterpret_cast<const T*>(data));
        else return storage_ops<N + 1, Ts...>::apply(n, data, v);
    }

    template<typename visitor>
    static typename visitor::result_type apply(int n, const void *data, const visitor& v) {
        if(n == N) return v(*reinterpret_cast<const T*>(data));
        else return storage_ops<N + 1, Ts...>::apply(n, data, v);
    }
};

template<int N>
struct storage_ops<N> {
    static void del(int n, void *data) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid.");
    }
    static void con(int n, void *data) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid." );
    }

    template<typename visitor>
    static typename visitor::result_type apply(int n, void *data, visitor& v) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid." );
    }
    template<typename visitor>
    static typename visitor::result_type apply(int n, void *data, const visitor& v) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid." );
    }
    template<typename visitor>
    static typename visitor::result_type apply(int n, const void *data, visitor& v) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid." );
    }
    template<typename visitor>
    static typename visitor::result_type apply(int n, const void *data, const visitor& v) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid." );
    }
};

template<typename X>
struct position<X> {
    static const int pos = -1;
};

template<typename X, typename... Ts>
struct position<X, X, Ts...> {
    static const int pos = 0;
};

template<typename X, typename T, typename... Ts>
struct position<X, T, Ts...> {
    static const int pos = position<X, Ts...>::pos != -1 ? position<X, Ts...>::pos + 1 : -1;
};

template<typename T, typename... Ts>
struct type_info<T&, Ts...> {
    static const bool no_reference_types = false;
    static const bool no_duplicates = position<T, Ts...>::pos == -1 && type_info<Ts...>::no_duplicates;
    static const size_t size = type_info<Ts...>::size > sizeof(T&) ? type_info<Ts...>::size : sizeof(T&);
    static const size_t count = 1 + type_info<Ts...>::count;
};

template<typename T, typename... Ts>
struct type_info<T, Ts...> {
    static const bool no_reference_types = type_info<Ts...>::no_reference_types;
    static const bool no_duplicates = position<T, Ts...>::pos == -1 && type_info<Ts...>::no_duplicates;
    static const size_t size = type_info<Ts...>::size > sizeof(T) ? type_info<Ts...>::size : sizeof(T&);
    static const size_t count = 1 + type_info<Ts...>::count;
};

template<>
struct type_info<> {
    static const bool no_reference_types = true;
    static const bool no_duplicates = true;
    static const size_t count = 0;
    static const size_t size = 0;
};

} // namespace impl

template<int L,typename Visitor,typename Data>
static const fc::array<typename Visitor::result_type(*)(Visitor&,Data),L>
      init_wrappers( Visitor& v, Data d, typename Visitor::result_type(**funcs)(Visitor&,Data) = 0)
{
   return fc::array<typename Visitor::result_type(*)(Visitor&,Data),L>();
}

template<int L,typename Visitor,typename Data,typename T, typename ... Types>
static const fc::array<typename Visitor::result_type(*)(Visitor&,Data),L>
      init_wrappers( Visitor& v, Data d, typename Visitor::result_type(**funcs)(Visitor&,Data) = 0 )
{
   fc::array<typename Visitor::result_type(*)(Visitor&,Data),L> result;
   if( !funcs ) funcs = result.begin();
   *funcs++ = [] ( Visitor& v, Data d ) { return v( *reinterpret_cast<T*>( d ) ); };
   init_wrappers<L,Visitor,Data,Types...>( v, d, funcs );
   return result;
}

template<int L,typename Visitor,typename Data>
static const fc::array<typename Visitor::result_type(*)(Visitor&,Data),L>
      init_const_wrappers( Visitor& v, Data d, typename Visitor::result_type(**funcs)(Visitor&,Data) = 0 )
{
   return fc::array<typename Visitor::result_type(*)(Visitor&,Data),L>();
}

template<int L,typename Visitor,typename Data,typename T, typename ... Types>
static const fc::array<typename Visitor::result_type(*)(Visitor&,Data),L>
      init_const_wrappers( Visitor& v, Data d, typename Visitor::result_type(**funcs)(Visitor&,Data) = 0 )
{
   fc::array<typename Visitor::result_type(*)(Visitor&,Data),L> result;
   if( !funcs ) funcs = result.begin();
   *funcs++ = [] ( Visitor& v, Data d ) { return v( *reinterpret_cast<const T*>( d ) ); };
   init_const_wrappers<L,Visitor,Data,Types...>( v, d, funcs );
   return result;
}

template<typename... Types>
class static_variant {
    static_assert(impl::type_info<Types...>::no_reference_types, "Reference types are not permitted in static_variant.");
    static_assert(impl::type_info<Types...>::no_duplicates, "static_variant type arguments contain duplicate types.");

    using tag_type = int64_t;
    tag_type _tag;
    char storage[impl::type_info<Types...>::size];

    template<typename X>
    void init(const X& x) {
        _tag = impl::position<X, Types...>::pos;
        new(storage) X(x);
    }

    template<typename X>
    void init(X&& x) {
        _tag = impl::position<X, Types...>::pos;
        new(storage) X( std::move(x) );
    }

    template<typename StaticVariant>
    friend struct impl::copy_construct;
    template<typename StaticVariant>
    friend struct impl::move_construct;
public:
    template<typename X>
    struct tag
    {
       static_assert(
         impl::position<X, Types...>::pos != -1,
         "Type not in static_variant."
       );
       static const int value = impl::position<X, Types...>::pos;
    };
    static_variant()
    {
       _tag = 0;
       impl::storage_ops<0, Types...>::con(0, storage);
    }

    template<typename... Other>
    static_variant( const static_variant<Other...>& cpy )
    {
       cpy.visit( impl::copy_construct<static_variant>(*this) );
    }
    static_variant( const static_variant& cpy )
    {
       cpy.visit( impl::copy_construct<static_variant>(*this) );
    }

    static_variant( static_variant&& mv )
    {
       mv.visit( impl::move_construct<static_variant>(*this) );
    }

    template<typename X>
    static_variant(const X& v) {
        static_assert(
            impl::position<X, Types...>::pos != -1,
            "Type not in static_variant."
        );
        init(v);
    }
    ~static_variant() {
       impl::storage_ops<0, Types...>::del(_tag, storage);
    }


    template<typename X>
    static_variant& operator=(const X& v) {
        static_assert(
            impl::position<X, Types...>::pos != -1,
            "Type not in static_variant."
        );
        this->~static_variant();
        init(v);
        return *this;
    }
    static_variant& operator=( const static_variant& v )
    {
       if( this == &v ) return *this;
       this->~static_variant();
       v.visit( impl::copy_construct<static_variant>(*this) );
       return *this;
    }
    static_variant& operator=( static_variant&& v )
    {
       if( this == &v ) return *this;
       this->~static_variant();
       v.visit( impl::move_construct<static_variant>(*this) );
       return *this;
    }
    friend bool operator == ( const static_variant& a, const static_variant& b )
    {
       return a.which() == b.which();
    }
    friend bool operator < ( const static_variant& a, const static_variant& b )
    {
       return a.which() < b.which();
    }

    template<typename X>
    X& get() {
        static_assert(
            impl::position<X, Types...>::pos != -1,
            "Type not in static_variant."
        );
        if(_tag == impl::position<X, Types...>::pos) {
            void* tmp(storage);
            return *reinterpret_cast<X*>(tmp);
        } else {
            FC_THROW_EXCEPTION( fc::assert_exception, "static_variant does not contain a value of type ${t}", ("t",fc::get_typename<X>::name()) );
           //     std::string("static_variant does not contain value of type ") + typeid(X).name()
           // );
        }
    }
    template<typename X>
    const X& get() const {
        static_assert(
            impl::position<X, Types...>::pos != -1,
            "Type not in static_variant."
        );
        if(_tag == impl::position<X, Types...>::pos) {
            const void* tmp(storage);
            return *reinterpret_cast<const X*>(tmp);
        } else {
            FC_THROW_EXCEPTION( fc::assert_exception, "static_variant does not contain a value of type ${t}", ("t",fc::get_typename<X>::name()) );
        }
    }
    template<typename visitor>
    typename visitor::result_type visit(visitor& v) {
        return visit( _tag, v, (void*) storage );
    }

    template<typename visitor>
    typename visitor::result_type visit(const visitor& v) {
        return visit( _tag, v, (void*) storage );
    }

    template<typename visitor>
    typename visitor::result_type visit(visitor& v)const {
        return visit( _tag, v, (const void*) storage );
    }

    template<typename visitor>
    typename visitor::result_type visit(const visitor& v)const {
        return visit( _tag, v, (const void*) storage );
    }

    template<typename visitor>
    static typename visitor::result_type visit( tag_type tag, visitor& v, void* data )
    {
        static const auto wrappers = init_wrappers<impl::type_info<Types...>::count,visitor,void*,Types...>( v, data );
        FC_ASSERT( tag >= 0 && tag < count(), "Unsupported type ${tag}!", ("tag",tag) );
        return wrappers[tag]( v, data );
    }

    template<typename visitor>
    static typename visitor::result_type visit( tag_type tag, const visitor& v, void* data )
    {
        static const auto wrappers = init_wrappers<impl::type_info<Types...>::count,const visitor,void*,Types...>( v, data );
        FC_ASSERT( tag >= 0 && tag < count(), "Unsupported type ${tag}!", ("tag",tag) );
        return wrappers[tag]( v, data );
    }

    template<typename visitor>
    static typename visitor::result_type visit( tag_type tag, visitor& v, const void* data )
    {
        static const auto wrappers = init_const_wrappers<impl::type_info<Types...>::count,visitor,const void*,Types...>( v, data );
        FC_ASSERT( tag >= 0 && tag < count(), "Unsupported type ${tag}!", ("tag",tag) );
        return wrappers[tag]( v, data );
    }

    template<typename visitor>
    static typename visitor::result_type visit( tag_type tag, const visitor& v, const void* data )
    {
        static const auto wrappers = init_const_wrappers<impl::type_info<Types...>::count,const visitor,const void*,Types...>( v, data );
        FC_ASSERT( tag >= 0 && tag < count(), "Unsupported type ${tag}!", ("tag",tag) );
        return wrappers[tag]( v, data );
    }

    static int count() { return impl::type_info<Types...>::count; }
    void set_which( tag_type w ) {
      FC_ASSERT( w >= 0 );
      FC_ASSERT( w < count() );
      this->~static_variant();
      _tag = w;
      impl::storage_ops<0, Types...>::con(_tag, storage);
    }

    tag_type which() const {return _tag;}
};

   struct from_static_variant 
   {
      variant& var;
      const uint32_t _max_depth;
      from_static_variant( variant& dv, uint32_t max_depth ):var(dv),_max_depth(max_depth){}

      typedef void result_type;
      template<typename T> void operator()( const T& v )const
      {
         to_variant( v, var, _max_depth );
      }
   };

   struct to_static_variant
   {
      const variant& var;
      const uint32_t _max_depth;
      to_static_variant( const variant& dv, uint32_t max_depth ):var(dv),_max_depth(max_depth){}

      typedef void result_type;
      template<typename T> void operator()( T& v )const
      {
         from_variant( var, v, _max_depth );
      }
   };


   template<typename... T> void to_variant( const fc::static_variant<T...>& s, fc::variant& v, uint32_t max_depth )
   {
      FC_ASSERT( max_depth > 0 );
      variants vars(2);
      vars[0] = s.which();
      s.visit( from_static_variant(vars[1], max_depth - 1) );
      v = std::move(vars);
   }
   template<typename... T> void from_variant( const fc::variant& v, fc::static_variant<T...>& s, uint32_t max_depth )
   {
      FC_ASSERT( max_depth > 0 );
      auto ar = v.get_array();
      if( ar.size() < 2 ) return;
      s.set_which( ar[0].as_uint64() );
      s.visit( to_static_variant(ar[1], max_depth - 1) );
   }

   template< typename... T > struct get_comma_separated_typenames;

   template<>
   struct get_comma_separated_typenames<>
   {
      static const char* names() { return ""; }
   };

   template< typename T >
   struct get_comma_separated_typenames<T>
   {
      static const char* names()
      {
         static const std::string n = get_typename<T>::name();
         return n.c_str();
      }
   };

   template< typename T, typename... Ts >
   struct get_comma_separated_typenames<T, Ts...>
   {
      static const char* names()
      {
         static const std::string n =
            std::string( get_typename<T>::name() )+","+
            std::string( get_comma_separated_typenames< Ts... >::names() );
         return n.c_str();
      }
   };

   template< typename... T >
   struct get_typename< static_variant< T... > >
   {
      static const char* name()
      {
         static const std::string n = std::string( "fc::static_variant<" )
            + get_comma_separated_typenames<T...>::names()
            + ">";
         return n.c_str();
      }
   };

} // namespace fc
