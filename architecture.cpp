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
//                                    \/                                      //
//                        model->simpleMethodImpl(Foo)                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

// Standard headers
#include <memory>
#include <iostream>
#include <typeinfo>
#include <exception>
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
struct no_##member##_tag {};                                                \
struct has_##member##_tag {};                                               \
                                                                            \
template<typename T>                                                        \
struct has_member_##member                                                  \
    : public std::integral_constant<bool, HasMember_##member<T>::RESULT> {  \
                                                                            \
  using tag = typename std::conditional<has_member_##member<T>::value,      \
                has_##member##_tag, no_##member##_tag>::type;               \
};


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
  virtual void method() = 0;
};

/* CLASS SimpleFoo ************************************************************/

// Forward declaration
template<typename T, typename M>
class SimpleFoo;

// Alias
template<typename T, typename M>
using SimpleFooPtr = std::shared_ptr<SimpleFoo<T, M>>;

/**
 * @class SimpleFoo
 * Simple implementation of Foo front-end
 */
template<typename T, typename M>
class SimpleFoo
    : public std::conditional<!std::is_void<typename M::base>::value,
               SimpleFoo<T, typename M::base>, Foo<T>>::type {
 public:
  // Alias
  using MPtr = std::shared_ptr<M>;

  // Constructor
  SimpleFoo(MPtr m = MPtr())
      : _m(std::move(m)) {
  }

 public:

  // Overriden methods
  void method() override {
    methodImpl(typename has_member_simpleMethod<M>::tag());
  }

 private:
  // Instance variables
  MPtr _m;

  // Concrete methods
  void methodImpl(no_simpleMethod_tag) {
    throw std::logic_error("Class don't have method!");
  }

  void methodImpl(has_simpleMethod_tag) {
    _m->template simpleMethod<T>(make_shared());
  }

  SimpleFooPtr<T, M> make_shared() {
    return std::static_pointer_cast<SimpleFoo<T, M>>(
      this->shared_from_this());
  }
};

/* CLASS CachedFoo ************************************************************/

// Forward declaration
template<typename T, typename M>
class CachedFoo;

// Alias
template<typename T, typename M>
using CachedFooPtr = std::shared_ptr<CachedFoo<T, M>>;

/**
 * @class CachedFoo
 * Cached implementation of Foo front-end
 */
template<typename T, typename M>
class CachedFoo
    : public std::conditional<!std::is_void<typename M::base>::value,
               CachedFoo<T, typename M::base>, Foo<T>>::type {
 public:
  // Alias
  using MPtr = std::shared_ptr<M>;
  using Cache = typename M::Cache;

  // Constructor
  CachedFoo(MPtr m = MPtr(), Cache cache = Cache())
      : _m(std::move(m)), _cache(std::move(cache)) {
  }

  // Overriden methods
  void method() override {
    methodImpl(typename has_member_cachedMethod<M>::tag());
  }

  // Concrete methods
  Cache cache() {
    return _cache;
  }

 private:
  // Instance variables
  MPtr _m;
  Cache _cache;

  // Concrete methods
  void methodImpl(no_cachedMethod_tag) {
    throw std::logic_error("Class don't have method!");
  }

  void methodImpl(has_cachedMethod_tag) {
    _m->template cachedMethod<T>(make_shared());
  }

  CachedFooPtr<T, M> make_shared() {
    return std::static_pointer_cast<CachedFoo<T, M>>(
      this->shared_from_this());
  }
};

/*
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 -------------------------------------------------------------------------------
                               HIERARCHY BACK-END
 -------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
*/

/* CLASS Target ***************************************************************/

class Target {
};

/* CLASS Spot *****************************************************************/

class Spot {
};

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
  virtual FooPtr<Target> targetFoo(bool cached) = 0;
  virtual FooPtr<Spot> spotFoo(bool cached) = 0;

  // Virtual methods
  template<typename T>
  void simpleMethod(SimpleFooPtr<T, Bar> simpleFoo);

  template<typename T>
  void cachedMethod(CachedFooPtr<T, Bar> simpleFoo);
};

template<> void Bar::simpleMethod<Target>(
    SimpleFooPtr<Target, Bar> simpleFoo) {
  std::cout << "Running simple for Target in Bar" << std::endl;
}

template<> void Bar::cachedMethod<Target>(
    CachedFooPtr<Target, Bar> cachedFoo) {
  std::cout << "Running cached for Target in Bar" << std::endl;
  std::cout << "Cache: " << typeid(cachedFoo->cache()).name() << std::endl;
}

template<> void Bar::simpleMethod<Spot>(
    SimpleFooPtr<Spot, Bar> simpleFoo) {
  std::cout << "Running simple for Spot in Bar" << std::endl;
}

template<> void Bar::cachedMethod<Spot>(
    CachedFooPtr<Spot, Bar> cachedFoo) {
  std::cout << "Running cached for Spot in Bar" << std::endl;
  std::cout << "Cache: " << typeid(cachedFoo->cache()).name() << std::endl;
}

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

  // Virtual methods
  template<typename T>
  void simpleMethod(SimpleFooPtr<T, BarDerived> simpleFoo);

  template<typename T>
  void cachedMethod(CachedFooPtr<T, BarDerived> simpleFoo);
};

template<> void BarDerived::simpleMethod<Target>(
    SimpleFooPtr<Target, BarDerived> simpleFoo) {
  std::cout << "Running simple for Target in BarDerived" << std::endl;
}

template<> void BarDerived::cachedMethod<Target>(
    CachedFooPtr<Target, BarDerived> cachedFoo) {
  std::cout << "Running cached for Target in BarDerived" << std::endl;
  std::cout << "Cache: " << typeid(cachedFoo->cache()).name() << std::endl;
}

template<> void BarDerived::simpleMethod<Spot>(
    SimpleFooPtr<Spot, BarDerived> simpleFoo) {
  std::cout << "Running simple for Spot in BarDerived" << std::endl;
}

template<> void BarDerived::cachedMethod<Spot>(
    CachedFooPtr<Spot, BarDerived> cachedFoo) {
  std::cout << "Running cached for Spot in BarDerived" << std::endl;
  std::cout << "Cache: " << typeid(cachedFoo->cache()).name() << std::endl;
}

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
