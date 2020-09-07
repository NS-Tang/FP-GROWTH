#pragma once

#include <vector>

#include "def.h"
class Trans
{
public:
	std::vector<item_t> m_trans;
public:
	std::vector<item_t> getTrans()const
	{
		return this->m_trans;
	}
};

//typedef std::vector<item_t> Trans;

