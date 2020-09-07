#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <algorithm>
#include <map>
#include <set>

#include "def.h"
#include "FPTree.h"

//这函数本来直接输出的，改成了存模式库，所以就一行。
inline void generatePattern(std::set<item_t>const& set, frq_t const support, PatternBase& generatePatternBase)
{
	generatePatternBase.emplace_back(set, support);
}

//FP-growth递归算法，照PPT里那个伪代码的格式写的
void fpGrowth(FPTree const& tree, std::set<item_t>const& alpha, frq_t min_sup_num, PatternBase& generatePatternBase)
{
	std::set<item_t> p;
	auto sup = tree.containsASinglePath(p);
	if (!p.empty())
	{
		//使用位标志生成幂集。
		//关于这里，生成完整的幂集应该i从0开始的，包含空集，PPT中也没有指明是否包含空集；
		//网上也有人这么做的，但是实际使用中发现这样生成了单独的条件模式基，
		//而单独的条件模式基通常会在下一个分支中（标记了注释：生成与项头表并集的频繁模式）生成，
		//且比这里生成的支持度计数高。也与手算得出的支持度计数符合，
		//所以这里从1开始循环，不包含所有位均为0的情况（不生成空集）。
		for (size_t i = 1; i < size_t(1) << p.size(); i++)
		{
			std::set<item_t> beta;
			size_t mask = 1;
			for (auto const item : p)
			{
				if (mask & i)
				{
					beta.insert(item);
				}
				mask <<= 1;
			}
			for (auto const item : alpha)
			{
				beta.insert(item);
			}
			generatePattern(beta, sup, generatePatternBase);
		}
	}
	else
	{
		for (auto const& supportAndA : tree.m_header)
		{
			auto support = supportAndA.first;
			auto a = supportAndA.second;

			auto beta = alpha;
			beta.insert(a);
			//生成与项头表并集的频繁模式
			//下面如果使用条件，则不生成频繁-1项集；如果条件恒为true，则生成频繁-1项集
			//由于还要算支持度置信度提升度，还是把频繁-1项集分出来好，频繁-1项集也可以直接调项头表提取
			if (!alpha.empty()/*true*/)
			{
				generatePattern(beta, support, generatePatternBase);
			}
			FPTree treeBeta(tree.constructPatternBase(a), min_sup_num);
			if (!treeBeta.isEmpty())
			{
				fpGrowth(treeBeta, beta, min_sup_num, generatePatternBase);
			}
		}
	}
}

//输出一个模式库
inline void outPatternBase(PatternBase const& patternBase)
{
	for (auto const& pattern : patternBase)
	{
		for (auto const& i : pattern.first)
		{
			std::cout << i << ' ';
		}
		std::cout << ':' << pattern.second << std::endl;
	}
}

//输出Rule, Support, Confidence, Lift
//不仅支持输出两项关联规则统计，同样支持输出任意多项的关联规则统计（虽然本作业中支持度计数3以上的只有两项及以下的）
inline void outRSCL(FPTree const& tree, PatternBase const& patternBase, double min_conf)
{
	auto allNum = tree.m_root.children.at(EMPTY_ITEM).node_sup;
	for (auto const& pattern : patternBase)
	{
		auto const& set = pattern.first;
		auto const sup = pattern.second;

		if (set.size() < 1)
		{
			throw "生成的模式库中有本不该有的空集，下面运算会陷入死循环。";
		}
		//在得出最后结果前尽量使用整数运算避免影响精度。
		auto supportPercent = double(sup * 100) / allNum;

		frq_t liftNumerator = sup;
		for (size_t i = 0; i != set.size() - 1; ++i)
		{
			liftNumerator *= allNum;
		}

		frq_t liftDenominator = 1;
		for (auto const i : set)
		{
			liftDenominator *= tree.m_itemAndInfoMap.at(i).item_sup;
		}
		auto lift = double(liftNumerator) / liftDenominator;

		for (auto const i : set)
		{
			auto confidencePercent = double(sup * 100) / tree.m_itemAndInfoMap.at(i).item_sup;
			//仅输出置信度大于min_conf的项
			if (confidencePercent >= min_conf * 100)
			{
				std::cout << i << "->";
				for (auto const j : set)
				{
					if (j != i)
					{
						std::cout << j << ' ';
					}
				}
				std::cout
					<< '('
					<< supportPercent << "%, "
					<< confidencePercent << "%, "
					<< lift
					<< ')' << std::endl;
			}
		}
	}
}

int main(int argc, char* argv[])
{
	using namespace std;
	//分数不太好控制台计算，为了精确用了整数，需输入支持度计数（支持度*原始数据条数）。
	frq_t min_sup_num;
	auto min_conf = atof(argv[2]);
	sscanf(argv[1], "%u", &min_sup_num);
	//原始数据，因为模式库的数据类型老改，影响赋值操作，所以单独写了一份
	std::map<TID_t, std::set<item_t>> motodata;
	motodata[100] = { 1, 2, 5 };
	motodata[200] = { 2, 4 };
	motodata[300] = { 2, 3 };
	motodata[400] = { 1, 2, 4 };
	motodata[500] = { 1, 3 };
	motodata[600] = { 2, 3 };
	motodata[700] = { 1, 3 };
	motodata[800] = { 1, 2, 3, 5 };
	motodata[900] = { 1, 2, 3 };
	//用原始数据初始化第一个模式库，实际应该使用外存储，并抽象一个带STL接口的类出来，不然基于范围的for循环等等一大堆东西不能用，
	//先硬编码凑活一下，算法没问题，生成FP-Tree只扫描了两次数据库。
	PatternBase dataset;
	for (auto const& i : motodata)
	{
		dataset.emplace_back(i.second, 1);
	}

	PatternBase generatePatternBase;//定义生成的模式库
	FPTree tree(dataset, min_sup_num);
	fpGrowth(tree, std::set<item_t>(), min_sup_num, generatePatternBase);

	//输出Rule, Support, Confidence, Lift
	outRSCL(tree, generatePatternBase, min_conf);
	std::cout << std::endl;

	std::cout << "附赠" << std::endl;
	std::cout << std::endl;
	std::cout << "频繁-1项集:" << std::endl;
	//输出频繁-1项集
	for (auto& i : tree.m_itemAndInfoMap)
	{
		std::cout << i.first << " :" << i.second.item_sup << endl;
	}
	std::cout << std::endl;

	//输出除了频繁-1项集以外其它的项集
	std::cout << "除了频繁-1项集以外其它的项集:" << std::endl;
	outPatternBase(generatePatternBase);

	return 0;
}

