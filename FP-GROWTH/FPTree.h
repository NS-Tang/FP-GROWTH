#pragma once

#include "def.h"

#include <map>
#include <list>
#include <set>

class FPTreeNode
{
public:
	frq_t node_sup = 0;//���нڵ��֧�ֶ�
	std::map<item_t, FPTreeNode>children;//���ӱ�

	//parent�ڵ�ĵ�������Ϊɶ���˵�����û��ָ�룬��Ϊ������ָ��һ��std::pair����һ����Ա���Ǵ˽ڵ��item������������ڵ���û��item����
	//��Ȼ��ָ��pair��ָ��Ҳ���ԣ����ǲ�̫�û�ȡ
	decltype(children)::const_iterator  parent;
};

class Item_info//��ͷ�����item�������Ϣ
{
public:
	frq_t item_sup = 0;//��ͷ���е�֧�ֶ�
	std::list <decltype(FPTreeNode::parent)> head;//����ͬitem���ڵ���������������ͷ
};

class FPTree
{
public:
	FPTreeNode m_root;//null�ڵ�����һ���ڵ㣬Ϊ��ͳһ����
	std::map<item_t, Item_info> m_itemAndInfoMap;//��ͷ��
	std::multimap<frq_t, item_t> m_header;//��frequency�������ͷ������
private:
	//��ʼ����ͷ��ʹ��item���򣩣���ͷ���item_sup��Ϊ�������������
	inline void initItemAndInfoMap(PatternBase const& dataset, frq_t const min_sup_num)
	{
		for (auto const& transAndFrq : dataset)
		{
			for (auto const item : transAndFrq.first)
			{
				this->m_itemAndInfoMap[item].item_sup += transAndFrq.second;
			}
		}
		//ɾ��������֧�ֶ�Ҫ��Ľڵ�
		for (auto iter = this->m_itemAndInfoMap.begin(); iter != this->m_itemAndInfoMap.end();)
		{
			if (iter->second.item_sup < min_sup_num)
			{
				iter = this->m_itemAndInfoMap.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}
	//��ʼ��ʹ��frequency����������item����������������ͷ���������ҡ���PPT��ͬ�����������ģ�����PPT��Ҫ�õ�ʱ��Ҳ�Ǵ���С�Ŀ�ʼ������
	//����FP-Tree��ʱ���ڱ����Ϻ�����Ѱ����>m_header����ɡ���Ȼ��������ͷ������frequency���Ǹ������š�
	inline void initHeader()
	{
		for (auto const& itemAndInfo : this->m_itemAndInfoMap)
		{
			this->m_header.insert(std::make_pair(itemAndInfo.second.item_sup, itemAndInfo.first));
		}
	}
public:
	inline FPTree(PatternBase const& dataset, frq_t const min_sup_num)
	{
		m_root.children[EMPTY_ITEM].node_sup = 0;
		//��һ��ɨ�����ݿ⣨������ģʽ�⣩
		this->initItemAndInfoMap(dataset, min_sup_num);
		this->initHeader();

		//�ڶ���ɨ�����ݿ⣨������ģʽ�⣩
		for (auto const& transAndFrq : dataset)
		{
			auto trans = transAndFrq.first;

			//���ɰ�frequency�����ģʽorderedTrans
			//����frequency���صģ����ܺ�PPT���������ɲ�̫һ�����е��ظ��Ľڵ���Ի��࿴����������������ɽ��һ���ġ�
			std::multimap<frq_t, item_t, std::greater<frq_t>> orderedTrans;
			for (auto const item : trans)
			{
				//���������item����ȫ���ģ����ܲ��������쳣��������ֱ������
				try
				{
					auto itemInfo = this->m_itemAndInfoMap.at(item);
					orderedTrans.insert(std::make_pair(itemInfo.item_sup, item));
				}
				catch (const std::out_of_range&) {}
			}

			//������
			if (!orderedTrans.empty())
			{
				m_root.children[EMPTY_ITEM].node_sup += transAndFrq.second;
				auto now = m_root.children.find(EMPTY_ITEM);
				for (auto const& frqAndItem : orderedTrans)
				{
					auto& children = now->second.children;
					bool isFirst = (children.find(frqAndItem.second) == children.end());
					children[frqAndItem.second].node_sup += transAndFrq.second;
					auto next = children.find(frqAndItem.second);
					if (isFirst)
					{
						this->m_itemAndInfoMap.at(frqAndItem.second).head.push_back(next);
						next->second.parent = now;
					}
					now = next;
				}
			}
		}
	}

	//������
	inline bool isEmpty()
	{
		return m_root.children.at(EMPTY_ITEM).node_sup == 0;
	}

	//�ж������Ƿ�ֻ�е���·�������ص���·��Ҷ�ӽ���֧�ֶȼ��������ɵ�path�����ڲ���path����û��path��Ϊ�ռ�
	//���ڿ��ܴ��ڶ���ģʽֻ����һ��·���������ֻ�������������һ�ο�����
	inline frq_t containsASinglePath(std::set<item_t>& path)const
	{
		auto treeNodeIter = m_root.children.find(EMPTY_ITEM);
		if (treeNodeIter->second.children.size() == 1)
		{
			treeNodeIter = treeNodeIter->second.children.begin();
			for (; treeNodeIter->second.children.size() == 1; treeNodeIter = treeNodeIter->second.children.begin())
			{
				path.insert(treeNodeIter->first);
			}
		}

		if (treeNodeIter->second.children.empty())
		{
			path.insert(treeNodeIter->first);
		}
		else
		{
			path.clear();
		}
		return treeNodeIter->second.node_sup;
	}

	//��������ģʽ����������ͷ����������ģʽ�⣨���ڲ����������ǻ�������ģʽ��alpha�ģ���������ģʽ��ֻ����һ���¼ӵ�Ԫ�أ�
	inline PatternBase const constructPatternBase(item_t a)const
	{
		PatternBase condPb;
		for (auto const iter : this->m_itemAndInfoMap.at(a).head)
		{
			std::set<item_t> trans;
			if (iter->first != EMPTY_ITEM)
			{
				auto treeNodeIter = iter->second.parent;
				for (; treeNodeIter->first != EMPTY_ITEM; treeNodeIter = treeNodeIter->second.parent)
				{
					trans.insert(treeNodeIter->first);
				}
			}
			if (!trans.empty())
			{
				condPb.emplace_back(std::move(trans), iter->second.node_sup);
			}
		}
		return std::move(condPb);
	}
};
