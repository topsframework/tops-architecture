////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                            foo = model->foo()                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                               foo->method()                                //
//                                    \/                                      //
//                             foo->methodImpl()                              //
//                                    \/                                      //
//                          model->simpleMethod(Foo)                          //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

// Standard headers
#include <memory>
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
template<typename T, typename Type>                                            \
class HasType_##member                                                         \
{                                                                              \
 private:                                                                      \
  template<typename U> static std::true_type test(typename U::Type*);          \
  template<typename U> static std::false_type test(...);                       \
                                                                               \
 public:                                                                       \
  static constexpr bool value = decltype(test<T>(nullptr))::value              \
    || HasType_##member<typename T::Base, Type>::value;                        \
};                                                                             \
                                                                               \
/*- TAGS -------------------------------------------------------------------*/ \
                                                                               \
struct no_##member##_tag {};                                                   \
struct has_##member##_tag {};                                                  \
                                                                               \
/*- TYPE TRAIT -------------------------------------------------------------*/ \
                                                                               \
template<typename T, typename Type>                                            \
struct has_type_##member                                                       \
    : public std::integral_constant<bool, HasType_##member<T, Type>::value> {  \
  using tag = typename std::conditional<                                       \
                has_type_##member<T, Type>::value,                             \
                has_##member##_tag, no_##member##_tag                          \
              >::type;                                                         \
};

/*============================================================================*/
/*                           MEMBER METHOD DETECTOR                           */
/*============================================================================*/

#define GENERATE_HAS_MEMBER_METHOD(member)                                     \
                                                                               \
template<typename T, typename Dummy>                                           \
class HasMethod_##member;                                                      \
                                                                               \
/*- NON-CONST METHOD -------------------------------------------------------*/ \
                                                                               \
template<typename Result, typename... Params>                                  \
class HasMethod_##member<void, Result(Params...)> {                            \
 public:                                                                       \
  static constexpr bool value = false;                                         \
};                                                                             \
                                                                               \
template<typename T, typename Result, typename... Params>                      \
class HasMethod_##member<T, Result(Params...)>                                 \
{                                                                              \
 private:                                                                      \
  template<typename U, U> class Check;                                         \
                                                                               \
  template<typename U>                                                         \
  static std::true_type test(Check<Result(U::*)(Params...), &U::member>*);     \
                                                                               \
  template<typename U>                                                         \
  static std::false_type test(...);                                            \
                                                                               \
 public:                                                                       \
  static constexpr bool value = decltype(test<T>(nullptr))::value              \
    || HasMethod_##member<typename T::Base, Result(Params...)>::value;         \
};                                                                             \
                                                                               \
/*- CONST METHOD -----------------------------------------------------------*/ \
                                                                               \
template<typename Result, typename... Params>                                  \
class HasMethod_##member<void, const Result(Params...)> {                      \
 public:                                                                       \
  static constexpr bool value = false;                                         \
};                                                                             \
                                                                               \
template<typename T, typename Result, typename... Params>                      \
class HasMethod_##member<T, const Result(Params...)>                           \
{                                                                              \
 private:                                                                      \
  template<typename U, U> class Check;                                         \
                                                                               \
  template<typename U>                                                         \
  static std::true_type test(                                                  \
    Check<Result(U::*)(Params...) const, &U::member>*);                        \
                                                                               \
  template<typename U>                                                         \
  static std::false_type test(...);                                            \
                                                                               \
 public:                                                                       \
  static constexpr bool value = decltype(test<T>(nullptr))::value              \
    || HasMethod_##member<typename T::Base, const Result(Params...)>::value;   \
};                                                                             \
                                                                               \
/*- TAGS -------------------------------------------------------------------*/ \
                                                                               \
struct no_##member##_tag {};                                                   \
struct has_##member##_tag {};                                                  \
                                                                               \
/*- TYPE TRAIT -------------------------------------------------------------*/ \
                                                                               \
template<typename T, typename Dummy>                                           \
struct has_method_##member;                                                    \
                                                                               \
template<typename T, typename Result, typename... Params>                      \
struct has_method_##member<T, Result(Params...)>                               \
    : public std::integral_constant<                                           \
               bool, HasMethod_##member<T, Result(Params...)>::value> {        \
                                                                               \
  using tag = typename std::conditional<                                       \
                has_method_##member<T, Result(Params...)>::value,              \
                has_##member##_tag, no_##member##_tag                          \
              >::type;                                                         \
};

// Generate the above structure for the following list of methods:
GENERATE_HAS_MEMBER_METHOD(simpleMethod)
GENERATE_HAS_MEMBER_METHOD(cachedMethod)

/*
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 -------------------------------------------------------------------------------
                                MEMBER DELEGATOR
 -------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
*/

/*============================================================================*/
/*                         MEMBER DELEGATOR GENERATION                        */
/*============================================================================*/

#define GENERATE_METHOD_DELEGATOR(interface, implementation)                   \
                                                                               \
template<typename... Args>                                                     \
auto interface##Impl(no_##implementation##_tag, Args... args) const            \
    -> decltype(this->interface(args...)) {                                    \
  static_assert(is_base, "Class don't have method 'implementation'!");         \
  throw std::logic_error("Calling from base class with no 'implementation'");  \
}                                                                              \
                                                                               \
template<typename... Args>                                                     \
auto interface##Impl(has_##implementation##_tag, Args... args) const           \
    -> decltype(this->interface(args...)) {                                    \
                                                                               \
  using Klass = typename std::remove_cv<                                       \
    typename std::remove_pointer<decltype(this)>::type>::type;                 \
                                                                               \
  return _m->implementation(                                                   \
    std::static_pointer_cast<Klass>(                                           \
      const_cast<Klass*>(this)->shared_from_this()),                           \
    std::forward<Args>(args)...);                                              \
}

/*============================================================================*/
/*                           MEMBER DELEGATOR CALL                            */
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

#define CALL_METHOD_DELEGATOR(interface, implementation)                       \
                                                                               \
using Klass = typename std::remove_cv<                                         \
  typename std::remove_pointer<decltype(this)>::type>::type;                   \
using MethodType = typename inject_first_parameter<                            \
  std::shared_ptr<Klass>, decltype(&Klass::interface)>::type;                  \
                                                                               \
interface##Impl(typename has_method_##implementation<M, MethodType>::tag());

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
  virtual void method() const = 0;
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
  void method() const override {
    CALL_METHOD_DELEGATOR(method, simpleMethod)
  }

 private:
  // Instance variables
  MPtr _m;

  GENERATE_METHOD_DELEGATOR(method, simpleMethod)
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
  void method() const override {
    CALL_METHOD_DELEGATOR(method, cachedMethod)
  }

  // Concrete methods
  Cache cache() {
    return _cache;
  }

 private:
  // Instance variables
  MPtr _m;
  Cache _cache;

  GENERATE_METHOD_DELEGATOR(method, cachedMethod)
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

  // Virtual methods
  virtual void simpleMethod(SimpleFooPtr<Target, Derived> simpleFoo) const {
    std::cout << "Running simple for Target in BarCrtp" << std::endl;
  }

  virtual void cachedMethod(CachedFooPtr<Target, Derived> cachedFoo) const {
    std::cout << "Running cached for Target in BarCrtp" << std::endl;
    std::cout << "Cache: " << typeid(cachedFoo->cache()).name() << std::endl;
  }

  virtual void simpleMethod(SimpleFooPtr<Spot, Derived> simpleFoo) const {
    std::cout << "Running simple for Spot in BarCrtp" << std::endl;
  }

  virtual void cachedMethod(CachedFooPtr<Spot, Derived> cachedFoo) const {
    std::cout << "Running cached for Spot in BarCrtp" << std::endl;
    std::cout << "Cache: " << typeid(cachedFoo->cache()).name() << std::endl;
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
  void simpleMethod(SimpleFooPtr<Target, BarDerived> simpleFoo) const override {
    std::cout << "Running simple for Target in BarDerived" << std::endl;
  }

  void cachedMethod(CachedFooPtr<Target, BarDerived> cachedFoo) const override {
    std::cout << "Running cached for Target in BarDerived" << std::endl;
    std::cout << "Cache: " << typeid(cachedFoo->cache()).name() << std::endl;
  }

  void simpleMethod(SimpleFooPtr<Spot, BarDerived> simpleFoo) const override {
    std::cout << "Running simple for Spot in BarDerived" << std::endl;
  }

  void cachedMethod(CachedFooPtr<Spot, BarDerived> cachedFoo) const override {
    std::cout << "Running cached for Spot in BarDerived" << std::endl;
    std::cout << "Cache: " << typeid(cachedFoo->cache()).name() << std::endl;
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
