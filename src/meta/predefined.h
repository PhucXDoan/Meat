#pragma once

global constexpr struct { String name; Value value; } PREDEFINED_CONSTANTS[] =
	{
		{ LSTR("e"),   constant_e   },
		{ LSTR("pi"),  constant_pi  },
		{ LSTR("tau"), constant_tau }
	};

global constexpr struct { String name; Function* function; } PREDEFINED_FUNCTIONS[] =
	{
		{ LSTR("sin"), function_sin }
	};
