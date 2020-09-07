#pragma once

#include <set>
#include <vector>

typedef unsigned frq_t;//֧�ֶȵ���������

typedef unsigned item_t;//item����������

constexpr item_t EMPTY_ITEM = 0;//item��nullֵ

typedef unsigned TID_t;//TID����������

class Pattern//һ��ģʽ���������ͣ�����PatternBase���ȱ������std::map���ֲ����ʣ������ֲ��øģ�ֻ�ܰ�����װ��std::pair������
{
public:
	std::set<item_t> first;//ģʽ
	frq_t second;//ģʽ��֧�ֶ�
public:
	Pattern(std::set<item_t> param_first, frq_t param_second) :first(param_first), second(param_second) {}
};

typedef std::vector<Pattern> PatternBase;//ģʽ����������