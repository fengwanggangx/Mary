#ifndef __XMLNODE_H__
#define __XMLNODE_H__

#include <rapidxml.hpp>

namespace xml
{
	// 前向声明
	class XmlDocument;
	class XmlAttribute;

	// XML节点类
	class XmlNode
	{
	public:
		// 构造函数
		XmlNode(rapidxml::xml_node<>* pNode = nullptr);

		// 判断节点是否有效
		bool IsValid() const;

		// 获取节点名称
		std::string GetName() const;

		// 获取节点值
		std::string GetValue() const;

		// 设置节点值
		void SetValue(const std::string& strVal);

		// 获取第一个子节点
		XmlNode GetFirstChild(const std::string& strName = "") const;

		// 获取下一个兄弟节点
		XmlNode GetNextSibling(const std::string& strName = "") const;

		// 获取父节点
		XmlNode GetParent() const;
		// 获取所有子节点
		std::vector<XmlNode> GetChildren(const std::string& strName = "") const;
		// 创建子节点
		XmlNode CreateChild(const std::string& strName, const std::string& value = "");

		// 删除子节点
		void RemoveChild(XmlNode& node);
		// 获取第一个属性
		XmlAttribute GetFirstAttribute(const std::string& strName = "") const;

		// 获取所有属性
		std::vector<XmlAttribute> GetAttributes() const;

		// 添加属性
		void AddAttribute(const std::string& strName, const std::string& value);

		// 查找属性
		XmlAttribute FindAttribute(const std::string& strName) const;

		// 删除属性
		void RemoveAttribute(XmlAttribute& attr);

	private:
		rapidxml::xml_node<>* m_pNode{ nullptr };
		friend class XmlDocument;
	};

}

#endif
