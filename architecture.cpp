////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                            foo = model->foo()                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                             foo->method(msg)                               //
//                                    \/                                      //
//                           foo->method(tag, msg)                            //
//                                    \/                                      //
//                          model->method(foo, msg)                           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

// Standard headers
#include <memory>
#include <string>
#include <vector>
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

/*============================================================================*/
/*                      STATIC MEMBER FUNCTION DETECTOR                       */
/*============================================================================*/

#define GENERATE_HAS_STATIC_MEMBER_FUNCTION(member)                            \
                                                                               \
template<typename _Klass, typename Dummy>                                      \
class HasStaticMethod_##member;                                                \
                                                                               \
/*- STATIC METHOD ----------------------------------------------------------*/ \
                                                                               \
template<typename _Return, typename... _Args>                                  \
class HasStaticMethod_##member<void, _Return(_Args...)> {                      \
 public:                                                                       \
  static constexpr bool value = false;                                         \
};                                                                             \
                                                                               \
template<typename _Klass, typename _Return, typename... _Args>                 \
class HasStaticMethod_##member<_Klass, _Return(_Args...)>                      \
{                                                                              \
 private:                                                                      \
  template<typename _U, _U> class Check;                                       \
                                                                               \
  template<typename _U>                                                        \
  static std::true_type test(Check<_Return(*)(_Args...), &_U::member>*);       \
                                                                               \
  template<typename _U>                                                        \
  static std::false_type test(...);                                            \
                                                                               \
 public:                                                                       \
  static constexpr bool value = decltype(test<_Klass>(nullptr))::value         \
    || HasStaticMethod_##member<typename _Klass::Base,                         \
                                _Return(_Args...)>::value;                     \
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
struct has_static_method_##member;                                             \
                                                                               \
template<typename _Klass, typename _Return, typename... _Args>                 \
struct has_static_method_##member<_Klass, _Return(_Args...)>                   \
    : public std::integral_constant<bool,                                      \
               HasStaticMethod_##member<_Klass, _Return(_Args...)>::value> {   \
                                                                               \
  using tag = typename std::conditional<                                       \
                has_static_method_##member<_Klass, _Return(_Args...)>::value,  \
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
/*                           LATE EVALUATED BOOLEANS                          */
/*============================================================================*/

template<typename T>
struct always_true : public std::integral_constant<bool, true> {};

template<typename T>
struct always_false : public std::integral_constant<bool, false> {};

/*============================================================================*/
/*                              NON CONST RETURN                              */
/*============================================================================*/

template <typename T> struct non_const_return { typedef T type; };

template <typename T> struct non_const_return<T const  > { typedef T   type; };
template <typename T> struct non_const_return<T const& > { typedef T&  type; };
template <typename T> struct non_const_return<T const* > { typedef T*  type; };
template <typename T> struct non_const_return<T const&&> { typedef T&& type; };

template <typename T>
using non_const_return_t = typename non_const_return<T>::type;

template<typename T>
non_const_return_t<T> non_const_cast(T t) {
    return (non_const_return_t<T>) t;
}

/*============================================================================*/
/*                              CLASS OF POINTER                              */
/*============================================================================*/

template<typename T> struct class_of;

template<typename T> struct class_of<T*> {
  using type = typename std::remove_cv<
    typename std::remove_pointer<T*>::type>::type;
};

template<typename T>
using class_of_t = typename class_of<T>::type;

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
inline auto method##Impl(Args&&... args) const                                 \
    -> decltype(non_const_cast(this)->method(std::forward<Args>(args)...)) {   \
                                                                               \
  using MethodType = typename inject_first_parameter<                          \
    std::shared_ptr<class_of_t<decltype(this)>>,                               \
    decltype(&class_of_t<decltype(this)>::method)                              \
  >::type;                                                                     \
                                                                               \
  using DelegatedType = typename std::remove_cv<                               \
    typename std::remove_pointer<decltype(this->delegatedObject)>::type        \
  >::type::element_type;                                                       \
                                                                               \
  return method##Impl(                                                         \
    typename has_method_##method<DelegatedType, MethodType>::tag(),            \
    std::forward<Args>(args)...);                                              \
}                                                                              \
                                                                               \
template<typename... Args>                                                     \
inline auto method##Impl(Args&&... args)                                       \
    -> decltype(this->method(std::forward<Args>(args)...)) {                   \
  return (non_const_return_t<decltype(this->method(args...))>) (               \
    static_cast<const class_of_t<decltype(this)> *>(this)->method##Impl(       \
      std::forward<Args>(args)...));                                           \
}                                                                              \
                                                                               \
template<typename... Args>                                                     \
inline auto method##Impl(no_##method##_tag, Args&&... args) const              \
    -> decltype(non_const_cast(this)->method(std::forward<Args>(args)...)) {   \
  static_assert(always_false<decltype(this)>::value,                           \
                "Missing implementation of member function '" #method "'!");   \
  throw std::logic_error("Calling from base class with no 'method'");          \
}                                                                              \
                                                                               \
template<typename... Args>                                                     \
inline auto method##Impl(no_##method##_tag, Args&&... args)                    \
    -> decltype(this->method(std::forward<Args>(args)...)) {                   \
  return (non_const_return_t<decltype(this->method(args...))>) (               \
    static_cast<const class_of_t<decltype(this)>*>(this)->method##Impl(        \
      no_##method##_tag(), std::forward<Args>(args)...));                      \
}                                                                              \
                                                                               \
template<typename... Args>                                                     \
inline auto method##Impl(has_##method##_tag, Args&&... args) const             \
    -> decltype(non_const_cast(this)->method(std::forward<Args>(args)...)) {   \
  return (this->delegatedObject)->method(                                      \
    std::static_pointer_cast<class_of_t<decltype(this)>>(                      \
      const_cast<class_of_t<decltype(this)>*>(this)->shared_from_this()),      \
    std::forward<Args>(args)...);                                              \
}                                                                              \
                                                                               \
template<typename... Args>                                                     \
inline auto method##Impl(has_##method##_tag, Args&&... args)                   \
    -> decltype(this->method(std::forward<Args>(args)...)) {                   \
  return (non_const_return_t<decltype(this->method(args...))>) (               \
    static_cast<const class_of_t<decltype(this)>*>(this)->method##Impl(        \
      has_##method##_tag(), std::forward<Args>(args)...));                     \
}

/*============================================================================*/
/*                           MEMBER DELEGATOR CALL                            */
/*============================================================================*/

#define CALL_METHOD_DELEGATOR(method, ...)                                     \
do { return method##Impl(__VA_ARGS__); } while (false)

/*============================================================================*/
/*                STATIC MEMBER FUNCTION DELEGATOR GENERATION                 */
/*============================================================================*/

#define GENERATE_STATIC_MEMBER_FUNCTION_DELEGATOR(method, delegatedClass)      \
                                                                               \
GENERATE_HAS_STATIC_MEMBER_FUNCTION(method);                                   \
                                                                               \
template<typename... Args>                                                     \
inline auto method##Impl(Args&&... args) const                                 \
    -> decltype(non_const_cast(this)->method(std::forward<Args>(args)...)) {   \
                                                                               \
  using MethodType = typename std::remove_pointer<                             \
    std::shared_ptr<delegatedClass>(*)(                                        \
      std::shared_ptr<class_of_t<decltype(this)>>, Args...)                    \
  >::type;                                                                     \
                                                                               \
  using DelegatedType = delegatedClass;                                        \
                                                                               \
  return method##Impl(                                                         \
    typename has_static_method_##method<DelegatedType, MethodType>::tag(),     \
    std::forward<Args>(args)...);                                              \
}                                                                              \
                                                                               \
template<typename... Args>                                                     \
inline auto method##Impl(Args&&... args)                                       \
    -> decltype(this->method(std::forward<Args>(args)...)) {                   \
  return (non_const_return_t<                                                  \
            decltype(this->method(std::forward<Args>(args)...))>) (            \
    static_cast<const class_of_t<decltype(this)> *>(this)->method##Impl(       \
      std::forward<Args>(args)...));                                           \
}                                                                              \
                                                                               \
template<typename... Args>                                                     \
inline auto method##Impl(no_##method##_tag, Args&&... args) const              \
    -> decltype(non_const_cast(this)->method(std::forward<Args>(args)...)) {   \
                                                                               \
  static_assert(always_false<decltype(this)>::value,                           \
    "Missing implementation of static member function '" #method "'!");        \
                                                                               \
  using MethodType = typename std::remove_pointer<                             \
    std::shared_ptr<delegatedClass>(*)(                                        \
      std::shared_ptr<class_of_t<decltype(this)>>, Args...)                    \
  >::type;                                                                     \
                                                                               \
  throw std::logic_error(                                                      \
    std::string("Calling missing static member function '")                    \
    + typeid(MethodType).name() + std::string("'"));                           \
}                                                                              \
                                                                               \
template<typename... Args>                                                     \
inline auto method##Impl(no_##method##_tag, Args&&... args)                    \
    -> decltype(this->method(std::forward<Args>(args)...)) {                   \
  return (non_const_return_t<                                                  \
            decltype(this->method(std::forward<Args>(args)...))>) (            \
    static_cast<const class_of_t<decltype(this)>*>(this)->method##Impl(        \
      no_##method##_tag(), std::forward<Args>(args)...));                      \
}                                                                              \
                                                                               \
inline auto method##Impl(no_##method##_tag) const                              \
    -> decltype(non_const_cast(this)->method()) {                              \
  return method##Alt();                                                        \
}                                                                              \
                                                                               \
inline auto method##Impl(no_##method##_tag)                                    \
    -> decltype(this->method()) {                                              \
  return (non_const_return_t<decltype(this->method())>) (                      \
    static_cast<const class_of_t<decltype(this)>*>(this)                       \
      ->method##Impl(no_##method##_tag()));                                    \
}                                                                              \
                                                                               \
template<typename... Args>                                                     \
inline auto method##Impl(has_##method##_tag, Args&&... args) const             \
    -> decltype(non_const_cast(this)->method(std::forward<Args>(args)...)) {   \
  if (delegate()) {                                                            \
    return delegatedClass::method(                                             \
      std::static_pointer_cast<class_of_t<decltype(this)>>(                    \
        const_cast<class_of_t<decltype(this)>*>(this)->shared_from_this()),    \
      std::forward<Args>(args)...);                                            \
  }                                                                            \
  return method##Alt();                                                        \
}                                                                              \
                                                                               \
template<typename... Args>                                                     \
inline auto method##Impl(has_##method##_tag, Args&&... args)                   \
    -> decltype(this->method(std::forward<Args>(args)...)) {                   \
  return (non_const_return_t<                                                  \
            decltype(this->method(std::forward<Args>(args)...))>) (            \
    static_cast<const class_of_t<decltype(this)>*>(this)->method##Impl(        \
      has_##method##_tag(), std::forward<Args>(args)...));                     \
}

/*============================================================================*/
/*                   STATIC MEMBER FUNCTION DELEGATOR CALL                    */
/*============================================================================*/

#define CALL_STATIC_MEMBER_FUNCTION_DELEGATOR(method, ...)                     \
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
                                 FOO FRONT-END
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
class SimpleFoo : public Foo<T> {
 public:
  // Alias
  using MPtr = std::shared_ptr<M>;

  // Constructor
  SimpleFoo(MPtr m)
      : _m(std::move(m)) {
  }

  // Overriden methods
  void method(const std::string &msg) const override {
    CALL_METHOD_DELEGATOR(method, msg);
  }

 protected:
  // Instance variables
  MPtr _m;

 private:
  GENERATE_METHOD_DELEGATOR(method, _m)
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
class CachedFoo : public SimpleFoo<T, M> {
 public:
  // Alias
  using MPtr = std::shared_ptr<M>;
  using Cache = typename M::Cache;

  // Constructor
  CachedFoo(MPtr m, Cache cache = Cache())
      : SimpleFoo<T,M>(std::move(m)), _cache(std::move(cache)) {
  }

  // Overriden methods
  void method(const std::string &msg) const override {
    CALL_METHOD_DELEGATOR(method, msg);
  }

  // Concrete methods
  Cache cache() {
    return _cache;
  }

 protected:
  // Instance variables
  Cache _cache;

 private:
  GENERATE_METHOD_DELEGATOR(method, _m)
};

/*
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 -------------------------------------------------------------------------------
                               VISITOR FRONT-END
 -------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
*/

/* CLASS Visitor **************************************************************/

// Forward declaration
class Visitor;

// Alias
using VisitorPtr = std::shared_ptr<Visitor>;

// Forward declaration
class Baz;
class BarDerived;
class BarReusing;

class Visitor {
 public:
  // Purely virtual functions
  virtual void visit(std::shared_ptr<Baz> top) = 0;
  virtual void visit(std::shared_ptr<BarDerived> top) = 0;
  virtual void visit(std::shared_ptr<BarReusing> top) = 0;
};

/* CLASS Acceptor *************************************************************/

// Forward declaration
class Acceptor;

// Alias
using AcceptorPtr = std::shared_ptr<Acceptor>;

/**
 * @class Acceptor
 * Interface for implementation of Acceptor front-end
 */
class Acceptor : public std::enable_shared_from_this<Acceptor> {
 public:
  // Enum classes
  enum class traversal { pre_order, post_order };

  // Concrete methods
  void pre_order() {
    accept(traversal::pre_order);
  }

  void post_order() {
    accept(traversal::pre_order);
  }

  // Virtual methods
  virtual void accept(const traversal& type = traversal::post_order) = 0;
};

/* CLASS SimpleAcceptor *******************************************************/

// Forward declaration
template<typename M>
class SimpleAcceptor;

// Alias
template<typename M>
using SimpleAcceptorPtr = std::shared_ptr<SimpleAcceptor<M>>;

/**
 * @class SimpleAcceptor
 * Simple implementation of Acceptor front-end
 */
template<typename M>
class SimpleAcceptor : public Acceptor {
 public:
  // Alias
  using MPtr = std::shared_ptr<M>;

  // Constructor
  SimpleAcceptor(MPtr m, VisitorPtr visitor)
      : _m(std::move(m)), _visitor(visitor) {
  }

  // Overriden methods
  void accept(const Acceptor::traversal& type) override {
    CALL_METHOD_DELEGATOR(accept, type);
  }

  // Concrete methods
  VisitorPtr visitor() {
    return _visitor;
  }

 protected:
  // Instance variables
  MPtr _m;
  VisitorPtr _visitor;

 private:
  GENERATE_METHOD_DELEGATOR(accept, _m)
};

/*
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 -------------------------------------------------------------------------------
                               CREATOR ALGORITHMS
 -------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
*/

/* STRUCT creator_algorithm_tag ***********************************************/

struct creator_algorithm_tag {};

struct creator_carriage_tag : public creator_algorithm_tag {};
struct creator_newline_tag : public creator_algorithm_tag {};
struct creator_space_tag : public creator_algorithm_tag {};
struct creator_tab_tag : public creator_algorithm_tag {};

/*
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 -------------------------------------------------------------------------------
                               CREATOR FRONT-END
 -------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
*/

/* CLASS Creator **************************************************************/

// Forward declaration
template<typename T, typename M>
class Creator;

// Alias
template<typename T, typename M>
using CreatorPtr = std::shared_ptr<Creator<T, M>>;

/**
 * @class Creator
 * Interface for implementation of Creator front-end
 */
template<typename T, typename M>
class Creator : public std::enable_shared_from_this<Creator<T, M>> {
 public:
  // Alias
  using MPtr = std::shared_ptr<M>;

  // Purely virtual methods
  virtual std::vector<std::string>& words() = 0;
  virtual const std::vector<std::string>& words() const = 0;
  virtual void add_word(const std::string& word) = 0;

  // Concrete methods
  template<typename... Args>
  MPtr create(Args&&... args) {
    if (delegate())
      return M::create(this->shared_from_this(), std::forward<Args>(args)...);
    return create();
  }

 protected:
  // Purely virtual methods
  virtual MPtr create() = 0;
  virtual bool delegate() = 0;
};

/* CLASS SimpleCreator ********************************************************/

// Forward declaration
template<typename T, typename M>
class SimpleCreator;

// Alias
template<typename T, typename M>
using SimpleCreatorPtr = std::shared_ptr<SimpleCreator<T, M>>;

/**
 * @class SimpleCreator
 * Simple implementation of Creator front-end
 */
template<typename T, typename M>
class SimpleCreator : public Creator<T, M> {
 public:
  // Alias
  using MPtr = std::shared_ptr<M>;
  using Self = SimpleCreator<T, M>;
  using SelfPtr = std::shared_ptr<Self>;

  // Static methods
  template<typename... Args>
  static SelfPtr make(Args&&... args) {
    return SelfPtr(new Self(std::forward<Args>(args)...));
  }

  // Overriden methods
  std::vector<std::string>& words() override {
    return const_cast<std::vector<std::string>&>(
      static_cast<const Self *>(this)->words());
  }

  const std::vector<std::string>& words() const override {
    return _words;
  }

  void add_word(const std::string &word) override {
    _words.push_back(word);
  }

 protected:
  // Instance variables
  std::vector<std::string> _words;

  // Overriden methods
  MPtr create() override {
    throw std::logic_error("Should not be called");
  }

  bool delegate() override {
    return true;
  }
};

/* CLASS FixedCreator *********************************************************/

/**
 * @class FixedCreator
 * Fixed implementation of Creator front-end
 */
template<typename T, typename M>
class FixedCreator : public Creator<T, M> {
 public:
  // Alias
  using MPtr = std::shared_ptr<M>;
  using Self = FixedCreator;
  using SelfPtr = std::shared_ptr<Self>;

  // Static methods
  template<typename... Args>
  static SelfPtr make(Args&&... args) {
    return SelfPtr(new Self(std::forward<Args>(args)...));
  }

  // Overriden methods
  std::vector<std::string>& words() override {
    return const_cast<std::vector<std::string>&>(
      static_cast<const Self *>(this)->words());
  }

  const std::vector<std::string>& words() const override {
    throw std::logic_error("Should not be called");
  }

  void add_word(const std::string& /* word */) override {
    /* do nothing */
  }

 protected:
  // Instance variables
  MPtr _m;

  // Constructors
  FixedCreator(MPtr m) : _m(std::move(m)) {
  }

  // Overriden methods
  MPtr create() override {
    return M::make(*(_m.get()));
  }

  bool delegate() override {
    return false;
  }
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
class Top {
 public:
  // Destructor
  virtual ~Top() {}

  // Purely virtual methods
  virtual AcceptorPtr acceptor(VisitorPtr visitor) = 0;
  virtual void dump() = 0;
};

/* CLASS TopCrtp **************************************************************/

// Forward declaration
template<typename Derived>
class TopCrtp;

// Alias
template<typename Derived>
using TopCrtpPtr = std::shared_ptr<TopCrtp<Derived>>;

/**
 * @class TopCrtp
 * Implementation of visitor, using CRTP to inject methods in subclasses
 */
template<typename Derived>
class TopCrtp
    : public std::enable_shared_from_this<TopCrtp<Derived>>,
      public virtual Top {
 public:
  // Alias
  using Base = void;
  using Cache = int;
  using DerivedPtr = std::shared_ptr<Derived>;

  // Static methods
  static CreatorPtr<Target, Derived> targetCreator(DerivedPtr model) {
    return FixedCreator<Target, Derived>::make(model);
  }

  template<typename... Args>
  static CreatorPtr<Target, Derived> targetCreator(Args&&... args) {
    return SimpleCreator<Target, Derived>::make(std::forward<Args>(args)...);
  }

  static CreatorPtr<Target, Derived> spotCreator(DerivedPtr model) {
    return FixedCreator<Target, Derived>::make(model);
  }

  template<typename... Args>
  static CreatorPtr<Spot, Derived> spotCreator(Args&&... args) {
    return SimpleCreator<Spot, Derived>::make(std::forward<Args>(args)...);
  }

  // Overriden methods
  AcceptorPtr acceptor(VisitorPtr visitor) override {
    return std::make_shared<SimpleAcceptor<Derived>>(
      this->make_shared(), visitor);
  }

  void dump() override {
    std::cout << _text << std::endl;
  }

  // Virtual methods
  virtual void accept(SimpleAcceptorPtr<Derived> acceptor,
                      const Acceptor::traversal& /* type */) {
    acceptor->visitor()->visit(this->make_shared());
  }

 protected:
  // Instance variables
  std::string _text;

  // Constructors
  TopCrtp(const std::string &text = {})
    : _text(text) {
  }

  // Concrete methods
  DerivedPtr make_shared() {
    return std::static_pointer_cast<Derived>(
      static_cast<Derived *>(this)->shared_from_this());
  }
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
class Baz : public TopCrtp<Baz> {
 public:
  // Alias
  using Base = TopCrtp<Baz>;

  using Self = Baz;
  using SelfPtr = std::shared_ptr<Self>;

  // Static methods
  template<typename... Args>
  static SelfPtr make(Args&&... args) {
    return SelfPtr(new Self(std::forward<Args>(args)...));
  }

  static SelfPtr create(CreatorPtr<Target, Self> creator, creator_newline_tag) {
    std::string text;
    for (unsigned int i = 0; i < creator->words().size()-1; i++)
      text += creator->words()[i] + "\n";
    text += creator->words().back();
    return Self::make(text);
  }

  static SelfPtr create(CreatorPtr<Target, Self> creator, creator_space_tag) {
    std::string text;
    for (unsigned int i = 0; i < creator->words().size()-1; i++)
      text += creator->words()[i] + " ";
    text += creator->words().back();
    return Self::make(text);
  }

 protected:
  // Constructor inheritance
  using Base::TopCrtp;
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
class Bar : public virtual Top {
 public:
  // Destructor
  virtual ~Bar() {}

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
class BarCrtp : public TopCrtp<Derived>, public virtual Bar {
 public:
  // Alias
  using Base = TopCrtp<Derived>;

  // Overriding methods
  FooPtr<Target> targetFoo(bool cached = true) override {
    if (cached)
      return std::make_shared<CachedFoo<Target, Derived>>(this->make_shared());
    return std::make_shared<SimpleFoo<Target, Derived>>(this->make_shared());
  }

  FooPtr<Spot> spotFoo(bool cached = true) override {
    if (cached)
      return std::make_shared<CachedFoo<Spot, Derived>>(this->make_shared());
    return std::make_shared<SimpleFoo<Spot, Derived>>(this->make_shared());
  }

  void messageBroadcast(const std::string& msg) const {
    if (!msg.empty())
      std::cout << "Transmiting message: " << msg << std::endl;
  }

  // Virtual methods
  virtual void method(SimpleFooPtr<Target, Derived> /* simple_foo */,
                      const std::string &msg) const {
    std::cout << "Running simple for Target in BarCrtp" << std::endl;
    messageBroadcast(msg);
  }

  virtual void method(CachedFooPtr<Target, Derived> cached_foo,
                      const std::string &msg) const {
    std::cout << "Running cached for Target in BarCrtp" << std::endl;
    std::cout << "Cache: " << typeid(cached_foo->cache()).name() << std::endl;
    messageBroadcast(msg);
  }

  virtual void method(SimpleFooPtr<Spot, Derived> /* simple_foo */,
                      const std::string &msg) const {
    std::cout << "Running simple for Spot in BarCrtp" << std::endl;
    messageBroadcast(msg);
  }

  virtual void method(CachedFooPtr<Spot, Derived> cached_foo,
                      const std::string &msg) const {
    std::cout << "Running cached for Spot in BarCrtp" << std::endl;
    std::cout << "Cache: " << typeid(cached_foo->cache()).name() << std::endl;
    messageBroadcast(msg);
  }

 protected:
  // Constructor inheritance
  using Base::TopCrtp;
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
  // Alias
  using Base = BarCrtp<BarDerived>;
  using Cache = double;

  using Self = BarDerived;
  using SelfPtr = std::shared_ptr<Self>;

  // Static methods
  template<typename... Args>
  static SelfPtr make(Args&&... args) {
    return SelfPtr(new Self(std::forward<Args>(args)...));
  }

  static SelfPtr create(CreatorPtr<Target, Self> creator, creator_carriage_tag) {
    std::string text;
    for (unsigned int i = 0; i < creator->words().size()-1; i++)
      text += creator->words()[i] + "\r";
    text += creator->words().back();
    return Self::make(text);
  }

  static SelfPtr create(CreatorPtr<Target, Self> creator, creator_newline_tag) {
    std::string text;
    for (unsigned int i = 0; i < creator->words().size()-1; i++)
      text += creator->words()[i] + "\n";
    text += creator->words().back();
    return Self::make(text);
  }

  // Constructors
  BarDerived(const std::string &text = {},
             const std::vector<BarPtr>& bars = {})
      : BarCrtp(text), _bars(bars) {
  }

  // Overriden methods
  void accept(SimpleAcceptorPtr<BarDerived> acceptor,
              const Acceptor::traversal& type) override {
    if (type == Acceptor::traversal::pre_order) compose_accept(acceptor, type);
    acceptor->visitor()->visit(this->make_shared());
    if (type == Acceptor::traversal::post_order) compose_accept(acceptor, type);
  }

  void method(SimpleFooPtr<Target, BarDerived> /* simple_foo */,
              const std::string &msg) const override {
    std::cout << "Running simple for Target in BarDerived" << std::endl;
    messageBroadcast(msg);
  }

  void method(CachedFooPtr<Target, BarDerived> cached_foo,
              const std::string &msg) const override {
    std::cout << "Running cached for Target in BarDerived" << std::endl;
    std::cout << "Cache: " << typeid(cached_foo->cache()).name() << std::endl;
    messageBroadcast(msg);
  }

  void method(SimpleFooPtr<Spot, BarDerived> /* simple_foo */,
              const std::string &msg) const override {
    std::cout << "Running simple for Spot in BarDerived" << std::endl;
    messageBroadcast(msg);
  }

  void method(CachedFooPtr<Spot, BarDerived> cached_foo,
              const std::string &msg) const override {
    std::cout << "Running cached for Spot in BarDerived" << std::endl;
    std::cout << "Cache: " << typeid(cached_foo->cache()).name() << std::endl;
    messageBroadcast(msg);
  }

 private:
  // Instance variables
  std::vector<BarPtr> _bars;

  // Concrete methods
  void compose_accept(SimpleAcceptorPtr<BarDerived> acceptor,
                      const Acceptor::traversal& type) {
    for (auto bar : _bars)
      bar->acceptor(acceptor->visitor())->accept(type);
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
  // Alias
  using Base = BarCrtp<BarReusing>;

  using Self = BarReusing;
  using SelfPtr = std::shared_ptr<Self>;

  // Static methods
  template<typename... Args>
  static SelfPtr make(Args&&... args) {
    return SelfPtr(new Self(std::forward<Args>(args)...));
  }

  static SelfPtr create(CreatorPtr<Target, Self> creator, creator_newline_tag) {
    std::string text;
    for (unsigned int i = 0; i < creator->words().size()-1; i++)
      text += creator->words()[i] + "\r\n";
    text += creator->words().back();
    return Self::make(text);
  }

  static SelfPtr create(CreatorPtr<Target, Self> creator, creator_tab_tag) {
    std::string text;
    for (unsigned int i = 0; i < creator->words().size()-1; i++)
      text += creator->words()[i] + "\t";
    text += creator->words().back();
    return Self::make(text);
  }

 protected:
  // Constructor inheritance
  using Base::BarCrtp;
};

/*
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 -------------------------------------------------------------------------------
                              VISITOR IMPLEMENTATION
 -------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
*/

// Forward declaration
class ConcreteVisitor;

// Alias
using ConcreteVisitorPtr = std::shared_ptr<ConcreteVisitor>;

/**
 * @class ConcreteVisitor
 * Concrete implementation of main hierarchy visitor
 */
class ConcreteVisitor : public Visitor {
 public:
  // Static methods
  template<typename... Args>
  static ConcreteVisitorPtr make(Args&&... args) {
    return ConcreteVisitorPtr(new ConcreteVisitor(std::forward<Args>(args)...));
  }

  // Overriden methods
  void visit(std::shared_ptr<Baz> /* top */) override {
  }

  void visit(std::shared_ptr<BarDerived> top) override {
    top->targetFoo()->method();
  }

  void visit(std::shared_ptr<BarReusing> top) override {
    top->targetFoo()->method();
  }
};

/*
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
 -------------------------------------------------------------------------------
                                      MAIN
 -------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
*/

/* FUNCTION main **************************************************************/

int main(int /* argc */, char ** /* argv */) {

  /**/ std::cout << std::endl; /*---------------------------------------------*/

  std::cout << "######################" << std::endl;
  std::cout << "# Test Foo front-end #" << std::endl;
  std::cout << "######################" << std::endl;

  /**/ std::cout << std::endl; /*---------------------------------------------*/

  std::cout << "Test BarDerived" << std::endl;
  std::cout << "================" << std::endl;
  auto bar_derived = BarDerived::make();
  bar_derived->targetFoo(false)->method();
  bar_derived->targetFoo(true)->method();
  bar_derived->spotFoo(false)->method();
  bar_derived->spotFoo(true)->method();

  /**/ std::cout << std::endl; /*---------------------------------------------*/

  std::cout << "Test BarDerived casted to Bar" << std::endl;
  std::cout << "==============================" << std::endl;
  static_cast<BarPtr>(bar_derived)->targetFoo(false)->method();
  static_cast<BarPtr>(bar_derived)->targetFoo(true)->method();
  static_cast<BarPtr>(bar_derived)->spotFoo(false)->method();
  static_cast<BarPtr>(bar_derived)->spotFoo(true)->method();

  /**/ std::cout << std::endl; /*---------------------------------------------*/

  std::cout << "Test BarReusing" << std::endl;
  std::cout << "================" << std::endl;
  auto bar_reusing = BarReusing::make();
  bar_reusing->targetFoo(false)->method();
  bar_reusing->targetFoo(true)->method();
  bar_reusing->spotFoo(false)->method();
  bar_reusing->spotFoo(true)->method();

  /**/ std::cout << std::endl; /*---------------------------------------------*/

  std::cout << "Test BarReusing casted to Bar" << std::endl;
  std::cout << "==============================" << std::endl;
  static_cast<BarPtr>(bar_reusing)->targetFoo(false)->method();
  static_cast<BarPtr>(bar_reusing)->targetFoo(true)->method();
  static_cast<BarPtr>(bar_reusing)->spotFoo(false)->method();
  static_cast<BarPtr>(bar_reusing)->spotFoo(true)->method();

  /**/ std::cout << std::endl; /*---------------------------------------------*/

  std::cout << "##########################" << std::endl;
  std::cout << "# Test Visitor front-end #" << std::endl;
  std::cout << "##########################" << std::endl;

  /**/ std::cout << std::endl; /*---------------------------------------------*/

  std::cout << "Test ConcreteVisitor" << std::endl;
  std::cout << "=====================" << std::endl;

  auto composite = BarDerived::make(
    "", std::vector<BarPtr>{ BarDerived::make(), BarReusing::make() }
  );

  composite->acceptor(ConcreteVisitor::make())->accept();

  /**/ std::cout << std::endl; /*---------------------------------------------*/

  std::cout << "##########################" << std::endl;
  std::cout << "# Test Creator front-end #" << std::endl;
  std::cout << "##########################" << std::endl;

  /**/ std::cout << std::endl; /*---------------------------------------------*/

  std::cout << "Test SimpleCreatorStrategy with Baz" << std::endl;
  std::cout << "====================================" << std::endl;

  auto baz_simple_creator = Baz::targetCreator();
  baz_simple_creator->add_word("This");
  baz_simple_creator->add_word("is");
  baz_simple_creator->add_word("a");
  baz_simple_creator->add_word("text");

  auto simple_created_baz_with_newline
    = baz_simple_creator->create(creator_newline_tag{});
  simple_created_baz_with_newline->dump();

  auto simple_created_baz_with_space
    = baz_simple_creator->create(creator_space_tag{});
  simple_created_baz_with_space->dump();

  /**/ std::cout << std::endl; /*---------------------------------------------*/

  std::cout << "Test FixedCreatorStrategy with Baz" << std::endl;
  std::cout << "===================================" << std::endl;

  auto predefined_baz = Baz::make("Predefined text");
  auto baz_fixed_creator = Baz::targetCreator(predefined_baz);
  baz_fixed_creator->add_word("This");
  baz_fixed_creator->add_word("is");
  baz_fixed_creator->add_word("a");
  baz_fixed_creator->add_word("text");

  auto fixed_created_baz_with_newline
    = baz_fixed_creator->create(creator_newline_tag{});
  fixed_created_baz_with_newline->dump();

  auto fixed_created_baz_with_space
    = baz_fixed_creator->create(creator_space_tag{});
  fixed_created_baz_with_space->dump();

  /**/ std::cout << std::endl; /*---------------------------------------------*/

  std::cout << "Test SimpleCreatorStrategy with BarDerived" << std::endl;
  std::cout << "===========================================" << std::endl;

  auto bar_derived_simple_creator = BarDerived::targetCreator();
  bar_derived_simple_creator->add_word("This");
  bar_derived_simple_creator->add_word("is");
  bar_derived_simple_creator->add_word("a");
  bar_derived_simple_creator->add_word("text");

  auto simple_created_bar_derived_with_newline
    = bar_derived_simple_creator->create(creator_newline_tag{});
  simple_created_bar_derived_with_newline->dump();

  auto simple_created_bar_derived_with_carriage
    = bar_derived_simple_creator->create(creator_carriage_tag{});
  simple_created_bar_derived_with_carriage->dump();

  /**/ std::cout << std::endl; /*---------------------------------------------*/

  std::cout << "Test FixedCreatorStrategy with BarDerived" << std::endl;
  std::cout << "==========================================" << std::endl;

  auto predefined_bar_derived = BarDerived::make("Predefined text");
  auto bar_derived_fixed_creator
    = BarDerived::targetCreator(predefined_bar_derived);
  bar_derived_fixed_creator->add_word("This");
  bar_derived_fixed_creator->add_word("is");
  bar_derived_fixed_creator->add_word("a");
  bar_derived_fixed_creator->add_word("text");

  auto fixed_created_bar_derived_with_newline
    = bar_derived_fixed_creator->create(creator_newline_tag{});
  fixed_created_bar_derived_with_newline->dump();

  auto fixed_created_bar_derived_with_carriage
    = bar_derived_fixed_creator->create(creator_carriage_tag{});
  fixed_created_bar_derived_with_carriage->dump();

  /**/ std::cout << std::endl; /*---------------------------------------------*/

  return 0;
}
