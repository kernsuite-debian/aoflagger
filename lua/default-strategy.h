#ifndef DATA_DEFAULT_STRATEGY_H
#define DATA_DEFAULT_STRATEGY_H

// The file default-strategy.cpp is generated with a run of 'xxd -i', to make it
// possible to include the default strategy literally in the flagger. I do this
// because not all platforms install data files in a standardized place when
// using installing in a prefix. This way, at least the default strategy is
// always available.

extern unsigned char data_strategies_generic_default_lua[];

extern unsigned int data_strategies_generic_default_lua_len;

#endif
