global constexpr Value constant_e   = { 2.7182818284f };
global constexpr Value constant_pi  = { 3.1415926535f };
global constexpr Value constant_tau = { 6.2831853071f };

internal Value function_sin(FunctionArgumentNode* arguments)
{
	ASSERT(!arguments->next_node);
	return { sinf(arguments->value) };
}

internal Value function_cos(FunctionArgumentNode* arguments)
{
	ASSERT(!arguments->next_node);
	return { cosf(arguments->value) };
}

internal Value function_tan(FunctionArgumentNode* arguments)
{
	ASSERT(!arguments->next_node);
	return { tanf(arguments->value) };
}

internal Value function_atan2(FunctionArgumentNode* arguments)
{
	ASSERT(!arguments->next_node->next_node);
	return { atan2f(arguments->value, arguments->next_node->value) };
}
