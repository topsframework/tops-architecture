////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                            foo = model->foo()                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                             foo->method(msg)                               //
//                                    \/                                      //
//                           foo->methodImpl(msg)                             //
//                                    \/                                      //
//                          model->method(Foo, msg)                           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

// Standard headers
#include <memory>
#include <string>
#include <iostream>
#include <exception>
#include <type_traits>

/*
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 -------------------------------------------------------------------------------
                                 MEMBER DETECTER
 -------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
*/

// These macro are aimed to create a member detector: a class and type trait
// that can be used to check when a type/method exists in a class. As there is
// an infinite number of names, it's impossible to create a standard type trait
// only with the resources the language provides. Given that, it's necessary to
// create a macro to automatically generate all classes and alias from a name
// given as parameter.

// The following macros creates:
// - Template classes that uses SFINAE and multiple inheritance to decide if
//   the member exists in the class or in one of its superclasses.
// - A struct inheriting from `std::integral_constant`, which have a trait
//   compliant with STL.
// - Two alias `has_##member_tag` and `no_##member_tag` to selectively create
//   methods by applying SFINAE on its parameters.

/*============================================================================*/
/*                            MEMBER TYPE DETECTOR                            */
/*============================================================================*/

#define GENERATE_HAS_MEMBER_TYPE(member)                                       \
                                                                               \
/*- TYPE DETECTOR ----------------------------------------------------------*/ \
                                                                               \
template<typename _Klass, typename _Type>                                      \
class HasType_##member                                                         \
{                                                                              \
 private:                                                                      \
  template<typename _U> static std::true_type test(typename _U::_Type*);       \
  template<typename _U> static std::false_type test(...);                      \
                                                                               \
 public:                                                                       \
  static constexpr bool value = decltype(test<_Klass>(nullptr))::value         \
    || HasType_##member<typename _Klass::Base, _Type>::value;                  \
};                                                                             \
                                                                               \
/*- TAGS -------------------------------------------------------------------*/ \
                                                                               \
struct no_##member##_tag {};                                                   \
struct has_##member##_tag {};                                                  \
                                                                               \
/*- TYPE TRAIT -------------------------------------------------------------*/ \
                                                                               \
template<typename _Klass, typename _Type>                                      \
struct has_type_##member                                                       \
    : public std::integral_constant<                                           \
               bool, HasType_##member<_Klass, _Type>::value> {                 \
  using tag = typename std::conditional<                                       \
                has_type_##member<_Klass, _Type>::value,                       \
                has_##member##_tag, no_##member##_tag                          \
              >::type;                                                         \
}

/*============================================================================*/
/*                           MEMBER METHOD DETECTOR                           */
/*============================================================================*/

#define GENERATE_HAS_MEMBER_METHOD(member)                                     \
                                                                               \
template<typename _Klass, typename Dummy>                                      \
class HasMethod_##member;                                                      \
                                                                               \
/*- NON-CONST METHOD -------------------------------------------------------*/ \
                                                                               \
template<typename _Return, typename... _Args>                                  \
class HasMethod_##member<void, _Return(_Args...)> {                            \
 public:                                                                       \
  static constexpr bool value = false;                                         \
};                                                                             \
                                                                               \
template<typename _Klass, typename _Return, typename... _Args>                 \
class HasMethod_##member<_Klass, _Return(_Args...)>                            \
{                                                                              \
 private:                                                                      \
  template<typename _U, _U> class Check;                                       \
                                                                               \
  template<typename _U>                                                        \
  static std::true_type test(Check<_Return(_U::*)(_Args...), &_U::member>*);   \
                                                                               \
  template<typename _U>                                                        \
  static std::false_type test(...);                                            \
                                                                               \
 public:                                                                       \
  static constexpr bool value = decltype(test<_Klass>(nullptr))::value         \
    || HasMethod_##member<typename _Klass::Base, _Return(_Args...)>::value;    \
};                                                                             \
                                                                               \
/*- CONST METHOD -----------------------------------------------------------*/ \
                                                                               \
template<typename _Return, typename... _Args>                                  \
class HasMethod_##member<void, const _Return(_Args...)> {                      \
 public:                                                                       \
  static constexpr bool value = false;                                         \
};                                                                             \
                                                                               \
template<typename _Klass, typename _Return, typename... _Args>                 \
class HasMethod_##member<_Klass, const _Return(_Args...)>                      \
{                                                                              \
 private:                                                                      \
  template<typename _U, _U> class Check;                                       \
                                                                               \
  template<typename _U>                                                        \
  static std::true_type test(                                                  \
    Check<_Return(_U::*)(_Args...) const, &_U::member>*);                      \
                                                                               \
  template<typename _U>                                                        \
  static std::false_type test(...);                                            \
                                                                               \
 public:                                                                       \
  static constexpr bool value = decltype(test<_Klass>(nullptr))::value         \
    || HasMethod_##member<typename _Klass::Base,                               \
                          const _Return(_Args...)>::value;                     \
};                                                                             \
                                                                               \
/*- TAGS -------------------------------------------------------------------*/ \
                                                                               \
struct no_##member##_tag {};                                                   \
struct has_##member##_tag {};                                                  \
                                                                               \
/*- TYPE TRAIT -------------------------------------------------------------*/ \
                                                                               \
template<typename _Klass, typename Dummy>                                      \
struct has_method_##member;                                                    \
                                                                               \
template<typename _Klass, typename _Return, typename... _Args>                 \
struct has_method_##member<_Klass, _Return(_Args...)>                          \
    : public std::integral_constant<                                           \
               bool, HasMethod_##member<_Klass, _Return(_Args...)>::value> {   \
                                                                               \
  using tag = typename std::conditional<                                       \
                has_method_##member<_Klass, _Return(_Args...)>::value,         \
                has_##member##_tag, no_##member##_tag                          \
              >::type;                                                         \
}

/*
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 -------------------------------------------------------------------------------
                                MEMBER DELEGATOR
 -------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
*/

// Thase macros are aimed to avoid replication of code when creating new
// methods for a given front-end. They require almost no knowledge about
// the front end class, but  that the front-end superclass -end inherits
// from std::enable_shared_from_this.

/*============================================================================*/
/*                          FIRST PARAMETER INJECTION                         */
/*============================================================================*/

template<typename Ptr, typename Dummy> struct inject_first_parameter;

template<typename Ptr, typename Klass, typename Result, typename... Args>
struct inject_first_parameter<Ptr, Result(Klass::*)(Args...)> {
  using type = Result(Ptr, Args...);
};

template<typename Ptr, typename Klass, typename Result, typename... Args>
struct inject_first_parameter<Ptr, Result(Klass::*)(Args...) const> {
  using type = const Result(Ptr, Args...);
};

/*============================================================================*/
/*                         MEMBER DELEGATOR GENERATION                        */
/*============================================================================*/

#define GENERATE_METHOD_DELEGATOR(method, delegatedObject)                     \
                                                                               \
GENERATE_HAS_MEMBER_METHOD(method);                                            \
                                                                               \
template<typename... Args>                                                     \
inline auto method##Impl(Args... args) const                                   \
    -> decltype(this->method(args...)) {                                       \
                                                                               \
  using Klass = typename std::remove_cv<                                       \
    typename std::remove_pointer<decltype(this)>::type>::type;                 \
  using MethodType = typename inject_first_parameter<                          \
    std::shared_ptr<Klass>, decltype(&Klass::method)>::type;                   \
                                                                               \
  using DelegatedType = typename std::remove_cv<                               \
    typename std::remove_pointer<decltype(delegatedObject)>::type>::type;      \
                                                                               \
  return method##Impl(                                                         \
    typename has_method_##method<                                              \
      typename DelegatedType::element_type, MethodType>::tag(),                \
    std::forward<Args>(args)...);                                              \
}                                                                              \
                                                                               \
template<typename... Args>                                                     \
inline auto method##Impl(no_##method##_tag, Args... args) const                \
    -> decltype(this->method(args...)) {                                       \
  static_assert(is_base, "Class don't have method 'method'!");                 \
  throw std::logic_error("Calling from base class with no 'method'");          \
}                                                                              \
                                                                               \
template<typename... Args>                                                     \
inline auto method##Impl(has_##method##_tag, Args... args) const               \
    -> decltype(this->method(args...)) {                                       \
                                                                               \
  using Klass = typename std::remove_cv<                                       \
    typename std::remove_pointer<decltype(this)>::type>::type;                 \
                                                                               \
  return (delegatedObject)->method(                                            \
    std::static_pointer_cast<Klass>(                                           \
      const_cast<Klass*>(this)->shared_from_this()),                           \
    std::forward<Args>(args)...);                                              \
}

/*============================================================================*/
/*                           MEMBER DELEGATOR CALL                            */
/*============================================================================*/

#define CALL_METHOD_DELEGATOR(method, delegatedObject, ...)                    \
do { return method##Impl(__VA_ARGS__); } while (false)

/*
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 -------------------------------------------------------------------------------
                                 COMMON CLASSES
 -------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
*/

/* CLASS Target ***************************************************************/

class Target {
};

/* CLASS Spot *****************************************************************/

class Spot {
};

/*
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 -------------------------------------------------------------------------------
                               HIERARCHY FRONT-END
 -------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
*/

/* CLASS Foo ******************************************************************/

// Forward declaration
template<typename T>
class Foo;

// Alias
template<typename T>
using FooPtr = std::shared_ptr<Foo<T>>;

/**
 * @class Foo
 * Interface for implementation of Foo front-end
 */
template<typename T>
class Foo : public std::enable_shared_from_this<Foo<T>> {
 public:
  // Virtual methods
  virtual void method(const std::string &msg = "") const = 0;
};

/* CLASS SimpleFoo ************************************************************/

// Forward declaration
template<typename T, typename M, bool is_base>
class SimpleFoo;

// Alias
template<typename T, typename M, bool is_base = false>
using SimpleFooPtr = std::shared_ptr<SimpleFoo<T, M, is_base>>;

/**
 * @class SimpleFoo
 * Simple implementation of Foo front-end
 */
template<typename T, typename M, bool is_base = false>
class SimpleFoo
    : public std::conditional<!std::is_void<typename M::Base>::value,
               SimpleFoo<T, typename M::Base, true>, Foo<T>>::type {
 public:
  // Alias
  using MPtr = std::shared_ptr<M>;

  // Constructor
  SimpleFoo(MPtr m = MPtr())
      : _m(std::move(m)) {
  }

 public:
  // Overriden methods
  void method(const std::string &msg) const override {
    CALL_METHOD_DELEGATOR(method, _m, msg);
  }

 private:
  // Instance variables
  MPtr _m;

  GENERATE_METHOD_DELEGATOR(method, _m)
};

/* CLASS CachedFoo ************************************************************/

// Forward declaration
template<typename T, typename M, bool is_base>
class CachedFoo;

// Alias
template<typename T, typename M, bool is_base = false>
using CachedFooPtr = std::shared_ptr<CachedFoo<T, M, is_base>>;

/**
 * @class CachedFoo
 * Cached implementation of Foo front-end
 */
template<typename T, typename M, bool is_base = false>
class CachedFoo
    : public std::conditional<!std::is_void<typename M::Base>::value,
               CachedFoo<T, typename M::Base, true>, Foo<T>>::type {
 public:
  // Alias
  using MPtr = std::shared_ptr<M>;
  using Cache = typename M::Cache;

  // Constructor
  CachedFoo(MPtr m = MPtr(), Cache cache = Cache())
      : _m(std::move(m)), _cache(std::move(cache)) {
  }

  // Overriden methods
  void method(const std::string &msg) const override {
    CALL_METHOD_DELEGATOR(method, _m, msg);
  }

  // Concrete methods
  Cache cache() {
    return _cache;
  }

 private:
  // Instance variables
  MPtr _m;
  Cache _cache;

  GENERATE_METHOD_DELEGATOR(method, _m)
};

/*
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 -------------------------------------------------------------------------------
                               HIERARCHY BACK-END
 -------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
*/

/* CLASS Top ******************************************************************/

// Forward declaration
class Top;

// Alias
using TopPtr = std::shared_ptr<Top>;

/**
 * @class Top
 * Top of the class hierarchy
 */
class Top : public std::enable_shared_from_this<Top> {
 public:
  using Base = void;
  using Cache = int;
};

/* CLASS Baz ******************************************************************/

// Forward declaration
class Baz;

// Alias
using BazPtr = std::shared_ptr<Baz>;

/**
 * @class Baz
 * Basic son of Top
 */
class Baz : public Top {
 public:
  using Base = Top;
};

/* CLASS Bar ******************************************************************/

// Forward declaration
class Bar;

// Alias
using BarPtr = std::shared_ptr<Bar>;

/**
 * @class Bar
 * Complex son of Top, with definition of new front-end
 */
class Bar : public Top {
 public:
  using Base = Top;

  // Purely virtual methods
  virtual FooPtr<Target> targetFoo(bool cached) = 0;
  virtual FooPtr<Spot> spotFoo(bool cached) = 0;
};

/* CLASS BarCrtp **************************************************************/

// Forward declaration
template<typename Derived>
class BarCrtp;

// Alias
template<typename Derived>
using BarCrtpPtr = std::shared_ptr<BarCrtp<Derived>>;

/**
 * @class BarCrtp
 * Implementation of front-end, using CRTP to inject methods in subclasses
 */
template<typename Derived>
class BarCrtp : public Bar {
 public:
  using Base = Bar;
  using DerivedPtr = std::shared_ptr<Derived>;

  // Overriding methods
  FooPtr<Target> targetFoo(bool cached = true) override {
    if (cached)
      return std::make_shared<CachedFoo<Target, Derived>>(make_shared());
    return std::make_shared<SimpleFoo<Target, Derived>>(make_shared());
  }

  FooPtr<Spot> spotFoo(bool cached = true) override {
    if (cached)
      return std::make_shared<CachedFoo<Spot, Derived>>(make_shared());
    return std::make_shared<SimpleFoo<Spot, Derived>>(make_shared());
  }

  void messageBroadcast(const std::string& msg) const {
    if (!msg.empty())
      std::cout << "Transmiting message: " << msg << std::endl;
  }

  // Virtual methods
  virtual void method(SimpleFooPtr<Target, Derived> simpleFoo,
                      const std::string &msg) const {
    std::cout << "Running simple for Target in BarCrtp" << std::endl;
    messageBroadcast(msg);
  }

  virtual void method(CachedFooPtr<Target, Derived> cachedFoo,
                      const std::string &msg) const {
    std::cout << "Running cached for Target in BarCrtp" << std::endl;
    std::cout << "Cache: " << typeid(cachedFoo->cache()).name() << std::endl;
    messageBroadcast(msg);
  }

  virtual void method(SimpleFooPtr<Spot, Derived> simpleFoo,
                      const std::string &msg) const {
    std::cout << "Running simple for Spot in BarCrtp" << std::endl;
    messageBroadcast(msg);
  }

  virtual void method(CachedFooPtr<Spot, Derived> cachedFoo,
                      const std::string &msg) const {
    std::cout << "Running cached for Spot in BarCrtp" << std::endl;
    std::cout << "Cache: " << typeid(cachedFoo->cache()).name() << std::endl;
    messageBroadcast(msg);
  }

 private:
  DerivedPtr make_shared() {
    return std::static_pointer_cast<Derived>(
      static_cast<Derived *>(this)->shared_from_this());
  }
};

/* CLASS BarDerived ***********************************************************/

// Forward declaration
class BarDerived;

// Alias
using BarDerivedPtr = std::shared_ptr<BarDerived>;

/**
 * @class BarDerived
 * Class "overriding" parent implementation
 */
class BarDerived : public BarCrtp<BarDerived> {
 public:
  using Base = BarCrtp<BarDerived>;
  using Cache = double;

  // Overriden methods
  void method(SimpleFooPtr<Target, BarDerived> simpleFoo,
              const std::string &msg) const override {
    std::cout << "Running simple for Target in BarDerived" << std::endl;
    messageBroadcast(msg);
  }

  void method(CachedFooPtr<Target, BarDerived> cachedFoo,
              const std::string &msg) const override {
    std::cout << "Running cached for Target in BarDerived" << std::endl;
    std::cout << "Cache: " << typeid(cachedFoo->cache()).name() << std::endl;
    messageBroadcast(msg);
  }

  void method(SimpleFooPtr<Spot, BarDerived> simpleFoo,
              const std::string &msg) const override {
    std::cout << "Running simple for Spot in BarDerived" << std::endl;
    messageBroadcast(msg);
  }

  void method(CachedFooPtr<Spot, BarDerived> cachedFoo,
              const std::string &msg) const override {
    std::cout << "Running cached for Spot in BarDerived" << std::endl;
    std::cout << "Cache: " << typeid(cachedFoo->cache()).name() << std::endl;
    messageBroadcast(msg);
  }
};

/* CLASS BarReusing ***********************************************************/

// Forward declaration
class BarReusing;

// Alias
using BarReusingPtr = std::shared_ptr<BarReusing>;

/**
 * @class BarReusing
 * Class reusing parent implementation
 */
class BarReusing : public BarCrtp<BarReusing> {
 public:
  using Base = BarCrtp<BarReusing>;
};

/*
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 -------------------------------------------------------------------------------
                                      MAIN
 -------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
*/

/* FUNCTION main **************************************************************/

int main(int argc, char **argv) {

  std::cout << std::endl;

  std::cout << "Test BarDerived" << std::endl;
  std::cout << "================" << std::endl;
  auto barDerived = std::make_shared<BarDerived>();
  barDerived->targetFoo(false)->method();
  barDerived->targetFoo(true)->method();
  barDerived->spotFoo(false)->method();
  barDerived->spotFoo(true)->method();

  std::cout << std::endl;

  std::cout << "Test BarDerived casted to Bar" << std::endl;
  std::cout << "==============================" << std::endl;
  static_cast<BarPtr>(barDerived)->targetFoo(false)->method();
  static_cast<BarPtr>(barDerived)->targetFoo(true)->method();
  static_cast<BarPtr>(barDerived)->spotFoo(false)->method();
  static_cast<BarPtr>(barDerived)->spotFoo(true)->method();

  std::cout << std::endl;

  std::cout << "Test BarReusing" << std::endl;
  std::cout << "================" << std::endl;
  auto barReusing = std::make_shared<BarReusing>();
  barReusing->targetFoo(false)->method();
  barReusing->targetFoo(true)->method();
  barReusing->spotFoo(false)->method();
  barReusing->spotFoo(true)->method();

  std::cout << std::endl;

  std::cout << "Test BarReusing casted to Bar" << std::endl;
  std::cout << "==============================" << std::endl;
  static_cast<BarPtr>(barReusing)->targetFoo(false)->method();
  static_cast<BarPtr>(barReusing)->targetFoo(true)->method();
  static_cast<BarPtr>(barReusing)->spotFoo(false)->method();
  static_cast<BarPtr>(barReusing)->spotFoo(true)->method();

  std::cout << std::endl;

  return 0;
}
