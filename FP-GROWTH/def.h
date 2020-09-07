#pragma once

#include <set>
#include <vector>

typedef unsigned frq_t;//支持度的数据类型

typedef unsigned item_t;//item的数据类型

constexpr item_t EMPTY_ITEM = 0;//item的null值

typedef unsigned TID_t;//TID的数据类型

class Pattern//一条模式的数据类型，由于PatternBase事先编码成了std::map发现不合适，代码又不好改，只能把它封装成std::pair那样了
{
public:
	std::set<item_t> first;//模式
	frq_t second;//模式的支持度
public:
	Pattern(std::set<item_t> param_first, frq_t param_second) :first(param_first), second(param_second) {}
};

typedef std::vector<Pattern> PatternBase;//模式库数据类型