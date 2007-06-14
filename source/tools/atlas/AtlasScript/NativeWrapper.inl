#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>

// Set the maximum number of function arguments that can be handled
#define MAX_ARGS 3

// (This is included inside the definition of class ScriptInterface)
public:
	// Varieties of comma-separated list to fit on the head/tail/whole of another comma-separated list
	#define NUMBERED_LIST_TAIL(z, i, data) ,data##i
	#define NUMBERED_LIST_HEAD(z, i, data) data##i,
	#define NUMBERED_LIST_BALANCED(z, i, data) BOOST_PP_COMMA_IF(i) data##i
	// Some other things
	#define TYPED_ARGS(z, i, data) , T##i a##i
	#define CONVERT_ARG(z, i, data) T##i a##i; if (! ScriptInterface::FromJSVal<T##i>(cx, argv[i], a##i)) return JS_FALSE;

	// List-generating macros, named roughly after their first list item
	#define TYPENAME_T0_HEAD(z, i) BOOST_PP_REPEAT_##z (i, NUMBERED_LIST_HEAD, typename T)
	#define TYPENAME_T0_TAIL(z, i) BOOST_PP_REPEAT_##z (i, NUMBERED_LIST_TAIL, typename T)
	#define T0(z, i) BOOST_PP_REPEAT_##z (i, NUMBERED_LIST_BALANCED, T)
	#define T0_HEAD(z, i) BOOST_PP_REPEAT_##z (i, NUMBERED_LIST_HEAD, T)
	#define T0_TAIL(z, i) BOOST_PP_REPEAT_##z (i, NUMBERED_LIST_TAIL, T)
	#define T0_A0(z, i) BOOST_PP_REPEAT_##z (i, TYPED_ARGS, ~)
	#define A0(z, i) BOOST_PP_REPEAT_##z (i, NUMBERED_LIST_BALANCED, a)
	#define A0_TAIL(z, i) BOOST_PP_REPEAT_##z (i, NUMBERED_LIST_TAIL, a)

	// Define RegisterFunction<TR, T0..., f>
	#define OVERLOADS(z, i, data) \
		template <typename TR, TYPENAME_T0_HEAD(z,i)  TR (*fptr) ( T0(z,i) )> \
		void RegisterFunction(const char* name) { \
			Register(name, call<TR, T0_HEAD(z,i)  fptr>, nargs<0 T0_TAIL(z,i)>()); \
		}

	BOOST_PP_REPEAT(MAX_ARGS, OVERLOADS, ~)
	#undef OVERLOADS

private:
	// JSNative-compatible function that wraps the function identified in the template argument list
	// (Definition comes later, since it depends on some things we haven't defined yet)
	#define OVERLOADS(z, i, data) \
		template <typename TR, TYPENAME_T0_HEAD(z,i)  TR (*fptr) ( T0(z,i) )> \
		static JSBool call(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);
	BOOST_PP_REPEAT(MAX_ARGS, OVERLOADS, ~)
	#undef OVERLOADS

	// Argument-number counter
	#define OVERLOADS(z, i, data) \
		template <int dummy TYPENAME_T0_TAIL(z,i)> /* add a dummy parameter so we still compile with 0 template args */ \
		static size_t nargs() { return i; }
	BOOST_PP_REPEAT(MAX_ARGS, OVERLOADS, ~)
	#undef OVERLOADS

}; // end of class ScriptInterface, because the following specialised structs
   // are not permitted inside non-namespace scopes


// ScriptInterface_NativeWrapper<T>::call(cx, rval, fptr, args...) will call fptr(args),
// and if T != void then it will store the result in rval:

// Templated on the return type so void can be handled separately
template <typename TR>
struct ScriptInterface_NativeWrapper {
	#define OVERLOADS(z, i, data) \
		template<TYPENAME_T0_HEAD(z,i)  typename f> \
		static void call(JSContext* cx, jsval& rval, f fptr  T0_A0(z,i)) { \
			rval = ScriptInterface::ToJSVal<TR>(cx, fptr(A0(z,i))); \
		}

	BOOST_PP_REPEAT(MAX_ARGS, OVERLOADS, ~)
	#undef OVERLOADS
};

// Overloaded to ignore the return value from void functions
template <>
struct ScriptInterface_NativeWrapper<void> {
	#define OVERLOADS(z, i, data) \
		template<TYPENAME_T0_HEAD(z,i)  typename f> \
		static void call(JSContext* /*cx*/, jsval& /*rval*/, f fptr  T0_A0(z,i)) { \
			fptr(A0(z,i)); \
		}
	BOOST_PP_REPEAT(MAX_ARGS, OVERLOADS, ~)
	#undef OVERLOADS
};

// JSNative-compatible function that wraps the function identified in the template argument list
#define OVERLOADS(z, i, data) \
	template <typename TR, TYPENAME_T0_HEAD(z,i)  TR (*fptr) ( T0(z,i) )> \
	JSBool ScriptInterface::call(JSContext* cx, JSObject* /*obj*/, uintN /*argc*/, jsval* argv, jsval* rval) { \
		(void)argv; /* avoid 'unused parameter' warnings */ \
		BOOST_PP_REPEAT_##z (i, CONVERT_ARG, ~) \
		ScriptInterface_NativeWrapper<TR>::call(cx, *rval, fptr  A0_TAIL(z,i)); \
		return JS_TRUE; \
	}
BOOST_PP_REPEAT(MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS

// Clean up our mess
#undef NUMBERED_LIST_TAIL
#undef NUMBERED_LIST_HEAD
#undef NUMBERED_LIST_BALANCED
#undef TYPED_ARGS
#undef CONVERT_ARG
#undef TYPENAME_T0_HEAD
#undef TYPENAME_T0_TAIL
#undef T0
#undef T0_HEAD
#undef T0_TAIL
#undef T0_A0
#undef A0
#undef A0_TAIL
