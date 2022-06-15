#pragma once

global constexpr struct { StringView name; Value value; } PREDEFINED_CONSTANTS[] =
	{
		{ STRING_VIEW_OF("e"), constant_e },
		{ STRING_VIEW_OF("pi"), constant_pi },
		{ STRING_VIEW_OF("tau"), constant_tau },
	};

global constexpr struct { StringView name; Function* function; } PREDEFINED_FUNCTIONS[] =
	{
		{ STRING_VIEW_OF("sin"), function_sin },
		{ STRING_VIEW_OF("cos"), function_cos },
		{ STRING_VIEW_OF("tan"), function_tan },
		{ STRING_VIEW_OF("atan2"), function_atan2 },
	};
