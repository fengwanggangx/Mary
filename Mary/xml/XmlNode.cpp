#include "XmlNode.h"
#include "Xml.h"
#include "rapidxml_print.hpp"
#include <sstream> 
#include "XmlAttribute.h"

namespace xml
{
	// XmlNode方法实现
	XmlAttribute XmlNode::GetFirstAttribute(const std::string& strName) const 
	{
		if (!m_pNode)
		{
			return XmlAttribute();
		}
		return XmlAttribute(xml::GetPtr(m_pNode->first_attribute(strName.c_str())));
	}

	std::vector<XmlAttribute> XmlNode::GetAttributes() const 
	{
		std::vector<XmlAttribute> result;
		if (!m_pNode)
		{
			return result;
		}
		rapidxml::xml_attribute<>* pAttr = xml::GetPtr(m_pNode->first_attribute());
		while (nullptr != pAttr)
		{
			result.emplace_back(pAttr);
			pAttr = xml::GetPtr(pAttr->next_attribute());
		}

		return result;
	}



	XmlNode::XmlNode(rapidxml::xml_node<>* pNode) : m_pNode(pNode) {}

	// 判断节点是否有效
	bool XmlNode::IsValid() const 
	{ 
		return nullptr != m_pNode; 
	}

	// 获取节点名称
	std::string XmlNode::GetName() const
	{
		if (!m_pNode)
		{
			return "";
		}
		return xml::ToString(m_pNode->name());
	}

	// 获取节点值
	std::string XmlNode::GetValue() const
	{
		if (!m_pNode)
		{
			return "";
		}
		return xml::ToString(m_pNode->value());
	}

	// 设置节点值
	void XmlNode::SetValue(const std::string& strVal)
	{
		if (m_pNode) 
		{
			std::string_view strNewVal = m_pNode->document()->allocate_string(strVal.c_str());
			m_pNode->value(strNewVal);
		}
	}

	// 获取第一个子节点
	XmlNode XmlNode::GetFirstChild(const std::string& strName) const 
	{
		if (!m_pNode)
		{
			return XmlNode();
		}
		return strName.empty() ? XmlNode(m_pNode->first_node().get()) : XmlNode(m_pNode->first_node(strName.c_str()).get());
	}

	// 获取下一个兄弟节点
	XmlNode XmlNode::GetNextSibling(const std::string& strName) const
	{
		if (!m_pNode)
		{
			return XmlNode();
		}
		return strName.empty() ? XmlNode(m_pNode->next_sibling().get()) : XmlNode(m_pNode->next_sibling(strName.c_str()).get());
	}

	// 获取父节点
	XmlNode XmlNode::GetParent() const
	{
		if (!m_pNode || !m_pNode->parent()) return XmlNode();
		return XmlNode(m_pNode->parent().get());
	}

	// 获取所有子节点
	std::vector<XmlNode> XmlNode::GetChildren(const std::string& strName) const
	{
		std::vector<XmlNode> ret;
		if (nullptr == m_pNode)
		{
			return ret;
		}
		auto ptr = strName.empty() ? m_pNode->first_node() : m_pNode->first_node(strName.c_str());
		rapidxml::xml_node<>* pChild = ptr.has_value() ? ptr.get() : nullptr;
		while (nullptr != pChild)
		{
			ret.emplace_back(pChild);
			ptr = strName.empty() ? pChild->next_sibling() : pChild->next_sibling(strName.c_str());
			pChild = ptr.has_value() ? ptr.get() : nullptr;
		}

		return ret;
	}

	// 创建子节点
	XmlNode XmlNode::CreateChild(const std::string& strName, const std::string& value) 
	{
		if (!m_pNode) return XmlNode();

		std::string_view nodeName = m_pNode->document()->allocate_string(strName.c_str());
		rapidxml::xml_node<>* newNode = m_pNode->document()->allocate_node(rapidxml::node_element, nodeName);

		if (!value.empty()) {
			std::string_view nodeValue = m_pNode->document()->allocate_string(value.c_str());
			newNode->value(nodeValue);
		}

		m_pNode->append_node(newNode);
		return XmlNode(newNode);
	}

	// 删除子节点
	void XmlNode::RemoveChild(XmlNode& node)
	{
		if ((nullptr == m_pNode) || (nullptr == node.m_pNode))
		{
			return;
		}
		if (node.m_pNode->parent() != m_pNode)
		{
			return;
		}
		m_pNode->remove_node(node.m_pNode);
		node.m_pNode = nullptr;
	}


	inline void XmlNode::AddAttribute(const std::string& strName, const std::string& value) 
	{
		if (!m_pNode) return;

		std::string_view attrName = m_pNode->document()->allocate_string(strName.c_str());
		std::string_view attrValue = m_pNode->document()->allocate_string(value.c_str());

		rapidxml::xml_attribute<>* attr = m_pNode->document()->allocate_attribute(attrName, attrValue);
		m_pNode->append_attribute(attr);
	}

	inline XmlAttribute XmlNode::FindAttribute(const std::string& strName) const 
	{
		if (!m_pNode)
		{
			return XmlAttribute();
		}

		rapidxml::xml_attribute<>* attr = m_pNode->first_attribute(strName.c_str()).get();
		return XmlAttribute(attr);
	}

	inline void XmlNode::RemoveAttribute(XmlAttribute& attr) 
	{
		if (m_pNode && attr.m_pAttr && attr.m_pAttr->parent() == m_pNode) 
		{
			m_pNode->remove_attribute(attr.m_pAttr);
			attr.m_pAttr = nullptr;
		}
	}

}