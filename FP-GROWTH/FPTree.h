#pragma once

#include "def.h"

#include <map>
#include <list>
#include <set>

class FPTreeNode
{
public:
	frq_t node_sup = 0;//树中节点的支持度
	std::map<item_t, FPTreeNode>children;//孩子表

	//parent节点的迭代器，为啥用了迭代器没用指针，因为迭代器指向一个std::pair，第一个成员才是此节点的item名，我这个树节点里没标item名。
	//当然用指向pair的指针也可以，就是不太好获取
	decltype(children)::const_iterator  parent;
};

class Item_info//项头表除了item以外的信息
{
public:
	frq_t item_sup = 0;//项头表中的支持度
	std::list <decltype(FPTreeNode::parent)> head;//把相同item树节点链接起来的链表头
};

class FPTree
{
public:
	FPTreeNode m_root;//null节点上面一个节点，为了统一操作
	std::map<item_t, Item_info> m_itemAndInfoMap;//项头表
	std::multimap<frq_t, item_t> m_header;//按frequency排序的项头表索引
private:
	//初始化项头表（使用item排序），项头表的item_sup作为下面排序的依据
	inline void initItemAndInfoMap(PatternBase const& dataset, frq_t const min_sup_num)
	{
		for (auto const& transAndFrq : dataset)
		{
			for (auto const item : transAndFrq.first)
			{
				this->m_itemAndInfoMap[item].item_sup += transAndFrq.second;
			}
		}
		//删除不符合支持度要求的节点
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
	//初始化使用frequency正序索引的item集。如果用排序的项头表就用这个找。和PPT不同我这个是正序的，反正PPT上要用的时候也是从最小的开始遍历。
	//构造FP-Tree的时候在编码上好像很难按这个>m_header排序吧。既然可以用项头表索引frequency，那个单独排。
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
		//第一次扫描数据库（或条件模式库）
		this->initItemAndInfoMap(dataset, min_sup_num);
		this->initHeader();

		//第二次扫描数据库（或条件模式库）
		for (auto const& transAndFrq : dataset)
		{
			auto trans = transAndFrq.first;

			//生成按frequency排序的模式orderedTrans
			//由于frequency有重的，可能和PPT中树的生成不太一样，有的重复的节点可以互相看做是替代，最终生成结果一样的。
			std::multimap<frq_t, item_t, std::greater<frq_t>> orderedTrans;
			for (auto const item : trans)
			{
				//由于这里的item还是全部的，可能产生访问异常，产生了直接跳过
				try
				{
					auto itemInfo = this->m_itemAndInfoMap.at(item);
					orderedTrans.insert(std::make_pair(itemInfo.item_sup, item));
				}
				catch (const std::out_of_range&) {}
			}

			//生成树
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

	//判树空
	inline bool isEmpty()
	{
		return m_root.children.at(EMPTY_ITEM).node_sup == 0;
	}

	//判断树中是否只有单独路径，返回单独路径叶子结点的支持度计数，生成的path保存在参数path里。如果没有path改为空集
	//由于可能存在多条模式只生成一条路径的情况，只能深度优先搜索一次看看。
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

	//根据条件模式基集合由项头表生成条件模式库（由于操作的树已是基于条件模式基alpha的，这里条件模式基只包含一个新加的元素）
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
