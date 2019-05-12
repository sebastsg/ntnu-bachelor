#pragma once

#include "platform.hpp"

namespace no::debug {

enum class message_type { message, warning, critical, info };

}

#if ENABLE_DEBUG

#include "io.hpp"

namespace no::debug {

void append(int index, message_type type, const char* file, const char* func, int line, const std::string& message);

}

# define ASSERT(EXPRESSION) \
		if (!(EXPRESSION)) { \
			CRITICAL(#EXPRESSION); \
			abort(); \
		}

#if COMPILER_MSVC
# define DEBUG(ID, TYPE, STR) \
		no::debug::append(ID, TYPE, __FILE__, __FUNCSIG__, __LINE__, STRING(STR))
#elif defined(COMPILER_GCC)
# define DEBUG(ID, TYPE, STR) \
		no::debug::append(ID, TYPE, __FILE__, __PRETTY_FUNCTION__, __LINE__, STRING(STR))
#endif

# define DEBUG_LIMIT(ID, TYPE, STR, LIMIT) \
		{ \
			static int COUNTER = 0; \
			if (++COUNTER <= (LIMIT)) { \
				DEBUG(ID, TYPE, "[" << COUNTER << "/" << LIMIT << "] " << STR); \
			} \
		}

#else
# define ASSERT(EXPRESSION) 
# define DEBUG(ID, TYPE, STR) 
# define DEBUG_LIMIT(ID, TYPE, STR, LIMIT) 
#endif

#define MESSAGE(STR)  DEBUG(0, no::debug::message_type::message, STR)
#define WARNING(STR)  DEBUG(0, no::debug::message_type::warning, STR)
#define CRITICAL(STR) DEBUG(0, no::debug::message_type::critical, STR)
#define INFO(STR)     DEBUG(0, no::debug::message_type::info, STR)

#define MESSAGE_LIMIT(STR, LIMIT)  DEBUG_LIMIT(0, no::debug::message_type::message, STR, LIMIT)
#define WARNING_LIMIT(STR, LIMIT)  DEBUG_LIMIT(0, no::debug::message_type::warning, STR, LIMIT)
#define CRITICAL_LIMIT(STR, LIMIT) DEBUG_LIMIT(0, no::debug::message_type::critical, STR, LIMIT)
#define INFO_LIMIT(STR, LIMIT)     DEBUG_LIMIT(0, no::debug::message_type::info, STR, LIMIT)

#define MESSAGE_X(ID, STR)  DEBUG(ID, no::debug::message_type::message, STR)
#define WARNING_X(ID, STR)  DEBUG(ID, no::debug::message_type::warning, STR)
#define CRITICAL_X(ID, STR) DEBUG(ID, no::debug::message_type::critical, STR)
#define INFO_X(ID, STR)     DEBUG(ID, no::debug::message_type::info, STR)
