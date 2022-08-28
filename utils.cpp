#ifndef GUTILS
# define GUTILS

#include <functional>

template<typename Callable> struct DeferedCall {
	Callable call;
	DeferedCall(Callable&& _call): call(std::move(_call)) {}
	~DeferedCall() { call(); }
};

#define deferVarName(line) DeferedCall defered_call_##line = [&]()
#define deferVarNameHelper(line) deferVarName(line)
#define deferDo deferVarNameHelper(__LINE__)

#endif
