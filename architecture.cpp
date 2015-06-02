////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                            foo = model->foo()                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                               foo->method()                                //
//                                    \/                                      //
//                             fooImpl->method()                              //
//                                    \/                                      //
//                           fooImpl->methodImpl()                            //
//                                    \/                                      //
//                        model->simpleMethod(fooImpl)                        //
//                                    \/                                      //
//                      model->simpleMethodImpl(fooImpl)                      //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

// Standard headers
#include <memory>
#include <iostream>
#include <typeinfo>
#include <type_traits>

/*
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 -------------------------------------------------------------------------------
                              IMPLEMENTATION HELPER
 -------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
*/

/* MACRO GENERATE_HAS_MEMBER **************************************************/

// This macro is aimed to create a member detector: a class and type trait
// that can be used to check when an attribute/method exists in a class.
// As there is an infinite number of names, it's impossible to create a
// standard type trait only with the resources the language provides. Given
// that, it's necessary to create a macro to automatically generate all
// classes and alias from a name given as parameter.

// The following macro creates:
// - A template class that uses SFINAE and multiple inheritance to decide if
//   the member exists in the class. In case it does, the inner class `Derived`
//   has two `member`s and its size is the same is a char[2]. This information
//   is to store in `RESULT` a boolean that indicates a member exists.
// - A struct inheriting from `std::integral_constant`, which have a trait
//   compliant with STL.
// - Two alias `has_##member` and `no_##member` to selectively create
//   methods by applying SFINAE on its parameters.

#define GENERATE_HAS_MEMBER(member)                                         \
                                                                            \
template <typename T>                                                       \
class HasMember_##member                                                    \
{                                                                           \
private:                                                                    \
    using Yes = char[2];                                                    \
    using  No = char[1];                                                    \
                                                                            \
    struct Fallback { int member; };                                        \
    struct Derived : T, Fallback { };                                       \
                                                                            \
    template<typename U> static No&  test (decltype(U::member)*);           \
    template<typename U> static Yes& test (U*);                             \
                                                                            \
public:                                                                     \
    static constexpr bool RESULT                                            \
      = sizeof(test<Derived>(nullptr)) == sizeof(Yes);                      \
};                                                                          \
                                                                            \
template<typename T>                                                        \
struct has_member_##member                                                  \
    : public std::integral_constant<bool, HasMember_##member<T>::RESULT> {  \
};                                                                          \
                                                                            \
template<typename Model>                                                    \
using has_##member = typename                                               \
  std::enable_if<has_member_##member<Model>::value                          \
                 && std::is_constructible<Model>::value, bool>::type;       \
                                                                            \
template<typename Model>                                                    \
using no_##member = typename                                                \
  std::enable_if<!has_member_##member<Model>::value                         \
                 || !std::is_constructible<Model>::value, bool>::type;

// Generate the above structure for the following list of methods:
GENERATE_HAS_MEMBER(simpleMethod)
GENERATE_HAS_MEMBER(cachedMethod)

/*
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 -------------------------------------------------------------------------------
                               HIERARCHY FRONT-END
 -------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
*/

/* CLASS FooImpl **************************************************************/

// Forward declaration
class FooImpl;

// Alias
using FooImplPtr = std::shared_ptr<FooImpl>;

/**
 * @class FooImpl
 * Interface for implementation of Foo front-end
 */
class FooImpl : public std::enable_shared_from_this<FooImpl> {
 public:
  // Virtual methods
  virtual void method() = 0;
};

/* CLASS SimpleFooImpl ********************************************************/

// Forward declaration
template<typename T>
class SimpleFooImpl;

// Alias
template<typename T>
using SimpleFooImplPtr = std::shared_ptr<SimpleFooImpl<T>>;

/**
 * @class SimpleFooImpl
 * Simple implementation of Foo front-end
 */
template<typename T>
class SimpleFooImpl
    : public std::conditional<!std::is_void<typename T::base>::value,
               SimpleFooImpl<typename T::base>, FooImpl>::type {
 public:
  // Alias
  using TPtr = std::shared_ptr<T>;

  // Constructor
  SimpleFooImpl(TPtr t = TPtr())
      : _t(std::move(t)) {
  }

  // Overriden methods
  void method() override {
    methodImpl();
  }

 private:
  // Instance variables
  TPtr _t;

  // Concrete methods
  template<typename U = T>
  void methodImpl(no_simpleMethod<U>* dummy = nullptr) {
    std::cout << "Doing nothing... (don't have method or not constructible)";
    std::cout << std::endl;
  }

  template<typename U = T>
  void methodImpl(has_simpleMethod<U>* dummy = nullptr) {
    _t->simpleMethod(std::shared_ptr<SimpleFooImpl<U>>(make_shared()));
  }

  SimpleFooImplPtr<T> make_shared() {
    return std::static_pointer_cast<SimpleFooImpl<T>>(this->shared_from_this());
  }
};

/* CLASS CachedFooImpl ********************************************************/

// Forward declaration
template<typename T>
class CachedFooImpl;

// Alias
template<typename T>
using CachedFooImplPtr = std::shared_ptr<CachedFooImpl<T>>;

/**
 * @class CachedFooImpl
 * Cached implementation of Foo front-end
 */
template<typename T>
class CachedFooImpl
    : public std::conditional<!std::is_void<typename T::base>::value,
               CachedFooImpl<typename T::base>, FooImpl>::type {
 public:
  // Alias
  using TPtr = std::shared_ptr<T>;
  using Cache = typename T::Cache;

  // Constructor
  CachedFooImpl(TPtr t = TPtr(), Cache cache = Cache())
      : _t(std::move(t)), _cache(std::move(cache)) {
  }

  // Overriden methods
  void method() override {
    methodImpl();
  }

  // Concrete methods
  Cache cache() {
    return _cache;
  }

 private:
  // Instance variables
  TPtr _t;
  Cache _cache;

  // Concrete methods
  template<typename U = T>
  void methodImpl(no_cachedMethod<U>* dummy = nullptr) {
    std::cout << "Doing nothing... (don't have method or not constructible)";
    std::cout << std::endl;
  }

  template<typename U = T>
  void methodImpl(has_cachedMethod<U>* dummy = nullptr) {
    _t->cachedMethod(std::shared_ptr<CachedFooImpl<U>>(make_shared()));
  }

  CachedFooImplPtr<T> make_shared() {
    return std::static_pointer_cast<CachedFooImpl<T>>(this->shared_from_this());
  }
};

/* CLASS Foo ******************************************************************/

// Forward declaration
class Foo;

// Alias
using FooPtr = std::shared_ptr<Foo>;

/**
 * @class Foo
 * Main class for Foo front-end
 */
class Foo {
 public:
  Foo(FooImplPtr impl) : _impl(std::move(impl)) {}

  virtual void method() {
    _impl->method();
  }

 private:
  FooImplPtr _impl;
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
  using base = void;
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
  using base = Top;
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
  using base = Top;

  // Purely virtual methods
  virtual FooPtr simpleFoo() = 0;
  virtual FooPtr cachedFoo() = 0;

  // Virtual methods
  virtual void simpleMethod(SimpleFooImplPtr<Bar> simpleFoo) {
    std::cout << "Running simple in Bar" << std::endl;
  }

  virtual void cachedMethod(CachedFooImplPtr<Bar> cachedFoo) {
    std::cout << "Running cached in Bar" << std::endl;
    std::cout << "Cache: " << typeid(cachedFoo->cache()).name() << std::endl;
  }
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
  using base = Bar;
  using DerivedPtr = std::shared_ptr<Derived>;

  // Overriding methods
  FooPtr simpleFoo() override {
    return std::make_shared<Foo>(
      std::make_shared<SimpleFooImpl<Derived>>(make_shared()));
  }

  FooPtr cachedFoo() override {
    return std::make_shared<Foo>(
      std::make_shared<CachedFooImpl<Derived>>(make_shared()));
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
  using base = Bar;
  using Cache = double;

  virtual void simpleMethod(SimpleFooImplPtr<BarDerived> simpleFoo) {
    std::cout << "Running simple in BarDerived" << std::endl;
  }

  virtual void cachedMethod(CachedFooImplPtr<BarDerived> cachedFoo) {
    std::cout << "Running cached in BarDerived" << std::endl;
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
  using base = Bar;
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
  barDerived->simpleFoo()->method();
  barDerived->cachedFoo()->method();

  std::cout << std::endl;

  std::cout << "Test BarDerived casted to Bar" << std::endl;
  std::cout << "==============================" << std::endl;
  static_cast<BarPtr>(barDerived)->simpleFoo()->method();
  static_cast<BarPtr>(barDerived)->cachedFoo()->method();

  std::cout << std::endl;

  std::cout << "Test BarReusing" << std::endl;
  std::cout << "================" << std::endl;
  auto barReusing = std::make_shared<BarReusing>();
  barReusing->simpleFoo()->method();
  barReusing->cachedFoo()->method();

  std::cout << std::endl;

  std::cout << "Test BarReusing casted to Bar" << std::endl;
  std::cout << "==============================" << std::endl;
  static_cast<BarPtr>(barReusing)->simpleFoo()->method();
  static_cast<BarPtr>(barReusing)->cachedFoo()->method();

  std::cout << std::endl;

  return 0;
}
